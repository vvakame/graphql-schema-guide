= Relay 各仕様解説

Relay@<fn>{relay}はFacebookが作成しているReact+GraphQLでフロントエンドの開発を行うためのライブラリです。
そして、Relayを便利に使うためのGraphQLスキーマに対する追加仕様がいくつか定義されています。
「私はApolloを使うから関係ないね！」という人にも便利な仕様ですので、ぜひ読んでもらいたいです。

//footnote[relay][@<href>{https://relay.dev/}]

本書ではめんどうなので、Relayが要求するサーバ仕様のことを"Relayの仕様"と呼ぶ場合があります。
Relayクライアントライブラリ自体の話は基本的にしません。

@<chapref>{github}で解説しているように、我々が見習うべきGitHubもRelayの追加仕様をほぼ踏襲しています。

Relayが求めるサーバ側（スキーマ）の仕様の狙いは次のとおりです。
これらについて、この章で解説していきます。

 * オブジェクトを再取得するため
 * Connectionを通じてページングを実装するため
 * Mutationの結果を予測可能にするため
 * クライアント側のもつキャッシュを適切に更新するため

== Global Object Identification

Global Object Identification@<fn>{object-identification}は全データをIDにより一意に特定可能にし、クライアントによる自動的なデータの再取得を可能にする仕様です。

//footnote[object-identification][@<href>{https://relay.dev/graphql/objectidentification.htm}]

@<code>{interface Node { id: ID! \}}という形式のインタフェースをこのために定義し、さらにクエリのrootに@<code>{node(id: ID!): Node}というフィールドを定義する必要があります。
具体的には@<list>{code/relay/globalObjectIdentification/schema.graphql}のようなスキーマになります。

//list[code/relay/globalObjectIdentification/schema.graphql][Global Object Identification スキーマ例]{
#@mapfile(../code/relay/globalObjectIdentification/schema.graphql)
type Query {
  node(id: ID!): Node
}

interface Node {
  id: ID!
}

type User implements Node {
  id: ID!
  name: String
}
#@end
//}

Nodeのidの値のみから、任意のデータを見つけてこられるようにIDを設計する必要があります。
これを実現するためには、開発初期から腰を据えてめんどくさい設計に取り組まねばなりません。
やっていきましょう。

技術書典Webの実例では、Userのidの値は、たとえば@<code>{"User:9999999999"}といった文字列になっています。
これはUserテーブル@<fn>{kind-not-table}のIDが9999999999のデータなわけです。
IDの値を単に@<code>{"9999999999"}にしてしまうと、どのテーブルのデータかを判別できません。

//footnote[kind-not-table][Datastoreなので本当はKind（カインド）なんですがわかりやすいようにテーブルと言いますね]

これは結構厄介な問題で、マイクロサービス構成を取り背後に複数のサーバAPIがある場合、どのサービスのどのテーブルのどのIDのデータかをIDに含めなければシステム全体で一意になりません。

GitHub v4 API@<fn>{github-explorer}を例に見てみましょう。
試しに、GoogleのorganizationのIDを見てみます（@<list>{code/relay/globalObjectIdentification/github-1.graphql}、@<list>{code/relay/globalObjectIdentification/github-1-result.json}）。
みると、@<code>{"MDEyOk9yZ2FuaXphdGlvbjEzNDIwMDQ="}という値ですね。
見るからにbase64っぽいので、デコードしてみると@<code>{012:Organization1342004}が得られます。
@<code>{Organization}はデータ種別、テーブル名とおそらくイコールでしょう。
@<code>{1342004}はデータベースIDと一致しているのでデータのIDでしょう。
先頭の@<code>{012}の部分は不明ですが、おそらくIDの書式バージョンを表しているのではないかと筆者は考えています。

//footnote[github-explorer][@<href>{https://docs.github.com/en/free-pro-team@latest/graphql/overview/explorer}]

//list[code/relay/globalObjectIdentification/github-1.graphql][Google organizationのIDを調べる]{
#@mapfile(../code/relay/globalObjectIdentification/github-1.graphql)
{
  organization(login: "Google") {
    __typename
    id
    databaseId
    login
  }
}
#@end
//}

//list[code/relay/globalObjectIdentification/github-1-result.json][レスポンス]{
{
  "data": {
    "organization": {
      "__typename": "Organization",
      "id": "MDEyOk9yZ2FuaXphdGlvbjEzNDIwMDQ=",
      "databaseId": 1342004,
      "login": "google"
    }
  }
}
//}

将来的な仕様変更に対する互換性（過去に生成したIDを内部でハンドリングできるようにするか）は頭の痛い問題です。
技術書典Webの方式は人間が見て意味がわかりやすく、クライアント側で合成するのが容易です。
一方、将来的にIDの構造に破壊的変更を加えようとした時、困りそうだというのは予想がつきます。
GitHubのやり方は人間には若干不親切ですが、将来的な破壊的変更を内部で吸収する余地があります。

グローバルなIDがどうやって成り立っているかを見たので、次はQueryのnodeフィールド側です。
引き続き、GitHub v4 APIの例を使って説明します。
nodeの引数として、さきほどのIDを指定すると任意のデータが取れるように設計されています（@<list>{code/relay/globalObjectIdentification/github-2.graphql}）。

//list[code/relay/globalObjectIdentification/github-2.graphql][nodeを使って同じデータを取得する]{
#@mapfile(../code/relay/globalObjectIdentification/github-2.graphql)
{
  node(id: "MDEyOk9yZ2FuaXphdGlvbjEzNDIwMDQ=") {
    __typename
    id
    ... on Organization {
      databaseId
      login
    }
  }
}
#@end
//}

取れるデータの型はNodeインタフェースの形なので、インラインフラグメントで実際の型を指定してやる必要があります。
得られるデータは先の@<list>{code/relay/globalObjectIdentification/github-1-result.json}とまったく同一です。

データを自動的に再取得できる機能は本当に必要なのか？というと筆者がApolloユーザであることもあり、実用上役に立っているのを見かけたことはありません。
しかしながら、この機能は開発時にかなり役に立ちます。
IDを知っているデータが欲しいとき、どのフィールドを使えば欲しいものが手に入るか？を調べるのは大変です。
もしかしたら、その手段がスキーマ上に存在していないことすらあるでしょう。
細かいことを気にしたり、不便さを感じるくらいであれば、ある程度定型的な実装を育てておけば需要を満たしてくれるnodeフィールドはリーズナブルな選択です。

Relayの定める仕様には存在しませんが、@<code>{nodes(ids: [ID!]!): [Node]!}というフィールドもついでに定義するとよいでしょう。
このフィールドは@<code>{node(id: ID!): Node}と実装を共通化できるでしょう。

== Cursor Connections

Cursor Connections@<fn>{cursor-connections}はカーソルを使ってリストを順にたどるための仕様、平たくいうとページングのための仕様です。
色々な実装方法を思いつきますが、広く使われている仕様があるのであれば、それに乗っかってしまうのが楽でよいでしょう。

//footnote[cursor-connections][@<href>{https://relay.dev/graphql/connections.htm}]

この仕様をざっくりいうと次のとおりです。

 1. リスト型のフィールドの代わりに、@<code>{Connection}サフィックスの型を作る
 2. @<code>{first: Int!}と@<code>{after: String}を引数に設ける@<fn>{connections-args}
 3. Connectionはcursorを持てる@<code>{Edge}と、ページング情報をもつ@<code>{PageInfo}をもつ

//footnote[connections-args][lastとbeforeもあるけどあんまり使わないし実装無理な場合が結構…]

Cursor Connectionsでは、単にリストを返す代わりにConnectionサフィックスをもつ型を使います。
ConnectionはRelayの仕様ではフィールドに@<code>{pageInfo: PageInfo!}と@<code>{edges: [XxxEdge]}を持ちます。
@<code>{PageInfo}型はページングに関する情報を持ち、@<code>{Edge}型は@<code>{cursor: String!}@<fn>{cursor-types}と@<code>{node: Xxx}を持ちます。

//footnote[cursor-types][仕様に注釈で"String!以外の型でもいいよ"とあります。筆者の場合、DBの仕様上nullを許容しないと処理コストが爆裂に悪化するので@<code>{cursor: String}にして使っています]
#@# OK zoncoen: 細かいんですが "String!" 以外でもいいよ！ですかね？
#@# OK vv: です！ 'This shows the cursor type as String!, other types are possible' です

Connection、Edge、PageInfoについて、仕様にないフィールドを追加するのは自由です。
実際に、GitHub v4 APIの場合、ConnectionにtotalCountフィールドを追加していたり、edgesの他にcursorを持たないnodesを定義していたりします。
筆者もedgesのcursorは必要なくて、pageInfoのendCursorで十分な場合がほとんどなので、真似してnodesを定義して運用しています@<fn>{apollo-v3-paging}。

//footnote[apollo-v3-paging][第2版執筆時点で、Apollot Client v3でnodesを利用すると困るシチュエーションが発生。後述。]

では、Cursor Connectionsの実際をGitHub v4 APIを例に見てみましょう（@<list>{code/relay/cursorConnections/github-1.graphql}）。
Repositoryのリストを返す場合、@<code>{[Repository]}を返す代わりに@<code>{RepositoryConnection}を返します。

//list[code/relay/cursorConnections/github-1.graphql][Cursor Connectionsのクエリ例]{
#@mapfile(../code/relay/cursorConnections/github-1.graphql)
{
  organization(login: "Google") {
    # repositories の型は RepositoryConnection!
    repositories(first: 2, after: "Y3Vyc29yOnYyOpHOAFi-oQ==") {
      # pageInfo は PageInfo!
      pageInfo {
        # ページングの前方向にページがあるか
        hasPreviousPage
        # ページングの前方向へのカーソル
        startCursor
        # ページングの次方向にページがあるか
        hasNextPage
        # ページングの次方向へのカーソル
        endCursor
      }
      # edges は [RepositoryEdge]
      edges {
        # このエッジを基点とした時のカーソル
        cursor
        # Repositoryのデータ本体
        node {
          id
          name
        }
      }
    }
  }
}
#@end
//}

//list[code/relay/cursorConnections/github-1.result.json][クエリ例の実行結果]{
#@mapfile(../code/relay/cursorConnections/github-1.result.json)
{
  "data": {
    "organization": {
      "repositories": {
        "pageInfo": {
          "hasPreviousPage": true,
          "startCursor": "Y3Vyc29yOnYyOpHOAFktDA==",
          "hasNextPage": true,
          "endCursor": "Y3Vyc29yOnYyOpHOAFz6sA=="
        },
        "edges": [
          {
            "cursor": "Y3Vyc29yOnYyOpHOAFktDA==",
            "node": {
              "id": "MDEwOlJlcG9zaXRvcnk1ODQ0MjM2",
              "name": "embed-dart-vm"
            }
          },
          {
            "cursor": "Y3Vyc29yOnYyOpHOAFz6sA==",
            "node": {
              "id": "MDEwOlJlcG9zaXRvcnk2MDkzNDg4",
              "name": "module-server"
            }
          }
        ]
      }
    }
  }
}
#@end
//}

次の2件を取得したい場合、pageInfoのendCursorかedgeのcursorをafterに指定して再度クエリを実行すればOKです。

この部分をスキーマに起こしてみると@<list>{code/relay/cursorConnections/mimicSchema.graphql}のようなスキーマになります。
実際はもっと複雑ですが、Cursor Connectionsの実装を示すには十分です。

//list[code/relay/cursorConnections/mimicSchema.graphql][簡略化したスキーマ]{
#@mapfile(../code/relay/cursorConnections/mimicSchema.graphql)
type Query {
  repositories(
    # 次方向ページングの時に使う
    first: Int
    after: String

    # 前方向ページングの時に使う
    last: Int
    before: String
  ): RepositoryConnection
}

type RepositoryConnection {
  pageInfo: PageInfo
  edges: [RepositoryEdge]
}

type PageInfo {
  hasPreviousPage: Boolean!
  startCursor: String
  hasNextPage: Boolean!
  endCursor: String
}

type RepositoryEdge {
  cursor: String!
  node: Repository
}

type Repository {
  id: ID!
  name: String!
}
#@end
//}

この仕様の優れたところはおおよそのDBで（少なくとも次へ方向は）実装可能なところです。
KVSでも実装可能でしょうし、RDBでも多少の工夫は必要だと思いますが実装可能でしょう@<fn>{but-author-using-kvs}。
カーソル方式のページングの実装方法については本書では扱わないので適当に調べてみてください。

//footnote[but-author-using-kvs][といいつつ筆者は普段KVSしか使わないのであった… RDBでも脳内では実装できてるから！]

この仕様で明記されていないのが、検索に使うパラメータをどうやって引数に指定するかです。
GitHub v4 APIの場合、firstやafterと同じ箇所、つまりフィールドの引数にprivaryやらorderByやらisForkが生えています。

第1版執筆時点では、技術書典Webでは、Cursor Connections以外のパラメータはinput要素にまとめてしまいました@<fn>{retrospective-input}。
@<code>{circles}フィールドの引数に@<code>{eventID: ID!}や@<code>{eventExhibitCourseID: ID}がある場合、それをまとめた@<code>{CirclesInput}を定義します。
@<code>{circles(first: Int, after: String, input: CirclesInput!): CircleExhibitInfoConnection!}という感じです。
#@# OK sonatard: inputが突然出てきたので何者！？と思ってしまいました。他のパラメーターまとめる専用の型を独自に作ったとわかるとすんなり入ってくると思いました。

//footnote[retrospective-input][後にこれを反省しinputを全部外してまわることになる]

第二版執筆時点では、@<code>{Query}で@<code>{input}を使うのはアンチパターンである、と認識を改めています。
第一版執筆時に「はてさてどっちが吉となるか？俺はinputを選ぶぜ！」とか書いてたんですけども敗北しました。
すみませんでした…。

なぜこれがアンチパターンだと考えるようになったかを説明します。

 1. フロント側的にはinputでまとめなくてもvariablesでまとまる
 2. 引数を明示的に指定するのは2-3がほとんどであまり多くならない
 3. デフォルト値の指定が活用できずつらい

それぞれ説明していきます。

1. input要素にまとめるとフロントエンド内でオブジェクトをまとめるのが楽になるのでは？と考えたのでした。
しかし、現実のフロントエンドではinputにまとめる前にvariablesに入力をまとめます。
さらに 2. にあるとおり、Queryに対して指定したい変数はだいたいの場合で2-3以下でした。
よって、要素数の多さとその取り回しを心配する必要を感じませんでした。

さらに、3. で悪くなる体験のほうが問題で、input objectの各フィールドにはデフォルト値が設定できません。
これにより、Queryへの引数を明示的に設定してもいいし、デフォルトにまかせてもよい…というフロントエンド中での省力化ができません。
さらに、スキーマの定義中でデフォルト値が設定できないのも不便です。
スキーマ内でデフォルト値が設定されていれば、@<code>{orderBy: OrderBy = SCORE}とある場合、デフォルトはスコア順だということがわかります。
これはドキュメントコメントに"デフォルトはスコア順ソートだよ"と書かれているよりも、遥かに明確に納得できます。

これらの利点を捨てるほうがデメリットが大きい、と判断し、第2版執筆時点ではQueryではinput objectを使わない方針に改めました。
ここでもGitHubがやっていない工夫はしないほうがよい、という学習が強化されました。

さて、話を戻してCursor Connectionsの仕様を無視すると発生しうる問題、そしてワザと無視してもよい場合について考えます。

GraphQLではクライアント側から自由なクエリを投げることができます。
つまり、サーバのリソースを食い尽くすようなクエリをワザと投げることができてしまいます。
GraphQLでセキュリティの話題が出るとき、だいたいはこの話題です。
この問題への対策として、クエリを実行せずに危険なクエリかどうかを判定できる必要があります。
それがクエリの複雑度（complexity）を事前に計算する方法です。
クエリの負荷の多寡の大部分は、DBへの負荷によって決まります。
つまり、何エンティティ取得する可能性があるかどうかを見積もればよい、となるわけですね。

GitHub v4 APIのリソース制限@<fn>{github-resource-limitations}が、まさにその考え方です。
@<list>{code/relay/cursorConnections/complexity.graphql}のようなクエリのコストを計算してみます。

//footnote[github-resource-limitations][@<href>{https://developer.github.com/v4/guides/resource-limitations/}]

//list[code/relay/cursorConnections/complexity.graphql][クエリの複雑度]{
#@mapfile(../code/relay/cursorConnections/complexity.graphql)
# リポジトリを20、その下のIssueを30、つまり 20+20×30 Entity最大で取得する
{
  viewer {
    # first: 20 なので最大20件取得できる
    repositories(first: 20) {
      nodes {
        name
        # first: 30 なので最大30件取得できる
        issues(first: 30) {
          nodes {
            title
            bodyText
          }
        }
      }
    }
  }
}
#@end
//}

GitHubのノードリミットの定義は、ログインユーザがもつリポジトリを20件取得し、さらにそれぞれのIssueを30件取得する場合、20件+20×30件の合計620ノードが取得されると事前に計算できます。
あとは、各々のサービスで1回あたりのコストをいくらまで許容するかを設計すればよいわけです。
ここでの重要なポイントは、事前に"何件"データを取得したいかが明確な点です。
Cursor Connectionsの仕様に従えば、@<code>{first}もしくは@<code>{last}を明示的に指定する必要があるため、自動的にこの条件が満たされます。

APIを外部に晒す必要があるシステムの場合、これらの制限を導入できるようスキーマを設計しておきましょう。

もちろん、常にConnectionが必要なわけではありません。
コスト計算を厳密にやらなければいけないのは、コストがかかる場合のみです。

リストの要素がGraphQLのスカラ型もしくはenumであれば、それ以上他の型に展開されることはありません。

第2版執筆時点で得た教訓として、リストの要素がスカラ型やenumではない場合、たとえそれがプログラム中にハードコーディングされるような、固定長のリストでも油断するべきではありません。
最初は他のオブジェクトへのリレーションを持たずとも、ある日突然別の型に派生する日が来るかもしれない。
それがGraphQLの強みであり、予想のつかなさでもあるのです。
このような場合、インメモリでのページング処理を実装し、Connectionを提供するようにしましょう。
Connectionにしなくてよいのはスカラ型とenumのリストだけです。

#@# OK gfx: Connectionはな〜モバイルアプリ的にはページングはこれでいいんだけど、デスクトップ向けのウェブアプリ的には、ページナビゲーション（"<< 1 2 3 ... 10 >>" みたいなやつ）を作れないから結構使いにくいんだよな〜もっとシンプルな仕様でいい気はするんだよな〜（独り言です）
#@# OK vv: 長年Datastoreを使ってる人間からするとページナビゲーションを採用するのは気が狂った選択だと思います！Gmailが実装してないUIは使っちゃいけないんだ！(暴論)

== Input Object Mutations

Input Object Mutations@<fn>{input-object-mutations}について説明します。
端的に説明すると、Mutationの引数のinputに@<code>{clientMutationId: String}をもたせ、サーバ側では同値を返り値に混ぜて返します。
これにより、リクエストとレスポンスの操作の紐付けを容易にします。

//footnote[input-object-mutations][@<href>{https://relay.dev/graphql/mutations.htm}]

…という仕様なのですが、最近のJavaScriptにはPromiseなどの非同期操作のためのAPIがあり、処理に連続性があります。
そのせいか、この仕様が活用されているのを見たことがないです。
実際、Relayでもモダンなバージョンでは不要だとか…？@<fn>{client-mutation-id-is-dead}

//footnote[client-mutation-id-is-dead][@<href>{https://github.com/facebook/relay/pull/2349}]

この仕様から得られる示唆もあるので、念の為仕様を解説していきましょう。
GitHub v4 APIを例に、@<list>{code/relay/inputObjectMutations/addReaction.graphql}のようなMutationを投げると@<list>{code/relay/inputObjectMutations/addReaction.result.json}の結果が返ってきます。

//list[code/relay/inputObjectMutations/addReaction.graphql][Mutation+clientMutationIdを投げる]{
#@mapfile(../code/relay/inputObjectMutations/addReaction.graphql)
mutation {
  addReaction(
    input: {
      # 任意の値を渡す UUIDとか
      clientMutationId: "foobarbuzz"
      subjectId: "MDU6SXNzdWU0ODc0OTQzNzk="
      content: LAUGH
    }
  ) {
    # クライアントが渡した値がそのまま返ってくる
    clientMutationId
    reaction {
      id
      content
    }
    subject {
      ... on Issue {
        id
        title
      }
    }
  }
}
#@end
//}

//list[code/relay/inputObjectMutations/addReaction.result.json][clientMutationIdがそのまま返ってくる]{
#@mapfile(../code/relay/inputObjectMutations/addReaction.result.json)
{
  "data": {
    "addReaction": {
      "clientMutationId": "foobarbuzz",
      "reaction": {
        "id": "MDg6UmVhY3Rpb240OTkzMDUyMQ==",
        "content": "LAUGH"
      },
      "subject": {
        "id": "MDU6SXNzdWU0ODc0OTQzNzk=",
        "title": "本の実験場"
      }
    }
  }
}
#@end
//}

このMutationは@<code>{addReaction(input: AddReactionInput!): AddReactionPayload}というシグニチャです。
@<code>{AddReactionPayload}は@<code>{clientMutationId: String}、@<code>{reaction: Reaction}、@<code>{subject: Reactable}の3つのフィールドを持っています。

ここで注目したいのは、@<code>{addReaction}の値が@<code>{AddReactionPayload}な点です。
@<code>{Reaction}ではないのです。
操作の結果を表す型を間に挟むことで、返り値の表現の幅を確保しています。
これによって、@<code>{clientMutationId}や@<code>{subject}といった追加の情報を返すことができています。
スキーマ設計の一貫性、拡張性のためにも、"もしclientMutationIdがあったら"どういう型にするべきかを考えるとよいでしょう。

なお、筆者は念の為各プロジェクトで@<code>{clientMutationId}を実装しています。
サボらないための命綱のようなものです。

#@# OK wawoon: clientMutationIdは自分は実装していないですが、mutationのinputにidempotencyKey（冪等キー）という名前で二重送信防止用のuuidをもたせています。
#@# OK wawoon: clientMutationIdをidempotency keyとして再利用する例はよく見られるので、mutationの冪等性についても追記していいかなと思いました。
#@# OK vv: Relayの章で扱う内容ではなさそうなので一旦放置にします！ 冪等性に関する会話が https://github.com/vvakame/graphql-schema-guide/pull/2 にあります（メモ）

== Mutations updater

これについては明確に仕様化されているわけではありません。
強いていえば@<href>{https://relay.dev/docs/en/mutations#range_add}でしょうか。
要旨を先に述べると効率的な画面の更新のためにサーバ側で行った追加・更新・削除といった操作をクライアント側でも把握できるようにしましょう、という話です。

サーバ側とやり取りを行ううちにクライアント側でキャッシュが育っていきます。
何らかの操作によりキャッシュが更新された際、リアクティブに（ある種自動的に）画面側に反映されます@<fn>{not-graphql}。
このため、Mutationでデータに変更を加えた後、クライアント側でデータがどう変化したのかをレスポンスから把握できるようスキーマを設計する必要があります。

//footnote[not-graphql][GraphQLクライアント全般の特徴というわけではないけど、RelayもApolloもそうだからまぁそうということでいいでしょ]

RelayでUpdaterで指定できる操作の設定は@<code>{NODE_DELETE}、@<code>{RANGE_ADD}、@<code>{RANGE_DELETE}の3種です。
それぞれ、データの削除、Connectionへの追加、Connectionからの削除に対応します。

Mutationによるデータの新規作成・更新はレスポンスを見れば自動的にキャッシュ（＝画面）に反映できます。
一方、先の3種の操作はレスポンスを見ただけではキャッシュの状態を最適な状態に保つことはできません。
それぞれ、どう対応するべきかを見ていきます。
#@# OK wawoon:（≒画面）のニアリイコールが機種依存文字みたいで, macおよびubuntuで文字化けしていました
#@# OK zoncoen: wawoon さんのコメントが生成物にふくまれてしまっていそう（別の行になってないからコメントになっていない）あとコメントの指摘はそうでpdfで表示できてないのを確認しました
#@# OK vv: イコール でも意味としておかしくないので置き換えました

データを削除する場合、一般的なユースケースでは削除したいIDをリクエストに含めるでしょう。
そのためクライアント側はドメイン知識があれば、処理が成功した時点でどのIDをキャッシュから消せばよいか分かります。
しかしながら複数のデータを消したパターンや、削除対象のIDがサーバ側で決まるパターンもあるでしょう。
このようなパターンと一貫性のある設計とするため、レスポンスには必ず削除したIDを含めるようにします。

技術書典Webのサークルチェックを解除する@<code>{removeCheckedCircle}のMutationを例にあげます。
サークルチェックはログインユーザ+対象サークルで操作できます。
そのため、チェックの解除を行ってもデータ本体である@<code>{CheckedCircleExhibit}のIDをクライアントが知る機会はありません。
そのままではクライアント側で削除されたデータのキャッシュを（サーバ側DBと同じ状態にするために）消すことができません。
これを解消するため、@<code>{removedCheckedCircleExhibitID}を設けてやります（@<list>{mutation-removed-id}）。

//list[mutation-removed-id][削除されたIDがわかるMutationの例]{
mutation {
  removeCheckedCircle(input: {
    circleExhibitInfoID: "CircleExhibitInfo:5726401537769472"
  }) {
    removedCheckedCircleExhibitID # 削除されたID
    checkedCircleExhibit {
      id
      circle {
        id
        name
      }
    }
  }
}
//}

これを実行すると@<list>{mutation-removed-id-result}のようにMutationが実行された結果、削除されたデータのIDが得られます。
クライアント側で該当のキャッシュを削除する処理を書くことができるようになりました。

//list[mutation-removed-id-result][Mutationの実行結果]{
{
  "data": {
    "removeCheckedCircle": {
      "removedCheckedCircleExhibitID":
        "CheckedCircleExhibit:5629499534213120:5726401537769472",
      "checkedCircleExhibit": {
        "id": "CheckedCircleExhibit:5629499534213120:5726401537769472",
        "circle": {
          "id": "CircleExhibitInfo:5726401537769472",
          "name": "たとえば村"
        }
      }
    }
  }
}
//}

残念ながら、削除したIDであることを示すコンセンサスの取れたフィールド名はありません。

Connectionへの追加、削除についてはドメイン知識が必要になるため、ライブラリに全自動的に処理させるのは難しいでしょう。
代わりに、今行ったMutationがどういう処理で、どういう結果が返ってきて、どうやったら既存のConnectionに追加、削除できるかを人間は理解できるはずです。
人間が頑張りましょう。
実際にどういう処理を書くかはクライアントライブラリごとに大きくことなるため、ここでは割愛します。

第2版執筆時点では、Apollo Clientではページング（Connection）のキャッシュ操作については別のやり方が登場してきています。
つまり、Mutations updaterはRelay専用の話題という側面が大きくなりました。

キャッシュに適切なデータの追加・更新を行うためには、Queryで使っているFragmentをMutationのレスポンスに対して再利用できるような構造でなくてはいけません。
REST APIだとつい空のJSONを返してしまったりする場合がありますが、GraphQLではちゃんと操作した該当データを返却するようにしましょう。
@<code>{NoopPayload}のような、データを何も含まないレスポンスの設計は明確にアンチパターンであるといえます。

#@# OK zoncoen: Mutations updater に関してもレスポンスの例があると初心者にも理解しやすいと思いました
#@# OK vv: 削除されたIDのくだりの例を追加してみました！他2つはガッツリクライアント側コードの話になるので割愛します…！

前述のとおり、レスポンスに削除したIDを含むデータ全体を返すのでもよいですし、削除したIDをもつフィールドをPayloadに明示的に含めるのでもよいでしょう。
GitHub v4 APIは前者を選んでいます。
第1版時点での筆者は、見てわかりやすい後者を選択しました。
第2版時点では、これもまた宗旨変えしGitHubと同じスタイルに落ち着いています。
これについても、なぜ考えを変えたのか、背景をお伝えします。

なぜGitHub方式にしたかというと、そのほうがフロントエンド側のコードを書かずに済むからです。
つまり、楽さ重視です。
画面の更新をするために必要なことは、サーバ側で消したはずのデータをキャッシュから消すこと…と思いがちですが、実はそうじゃない場合が多くあります。
表示したくないデータをキャッシュから消すのではなく、表示に使うデータ間の繋がりを断ち切ることで表示を行えないようにします。

あるサークルにチェックがついているか？という情報を表す場合、@<list>{checked-circle}のようなクエリで@<list>{checked-circle-resp}のようなデータが取れるとします。

//list[checked-circle][あるサークルにチェックをしたかどうか]{
query {
  node(id: "CircleExhibitInfo:5752754626625536") {
    ... on CircleExhibitInfo {
      id
      loginUserChecked {
        id
      }
    }
  }
}
//}

//list[checked-circle-resp][得られるデータ]{
{
  "data": {
    "node": {
      "id": "CircleExhibitInfo:5752754626625536",
      "loginUserChecked": {
        "id": "CheckedCircleExhibit:5629499534213120:5752754626625536"
      }
    }
  }
}
//}

このチェックを消すとき、@<list>{remove-checked-mutation}のように、サークルにチェックがなくなっていることが分かるようなレスポンスを要求します。
すると、@<list>{remove-checked-mutation-resp}のように、消したデータの箇所がnullになっている結果が得られます（当然）。

//list[remove-checked-mutation][消した後の正しい形が観測できるレスポンスを要求]{
mutation {
  removeCheckedCircle(input: {
    circleExhibitInfoID: "CheckedCircleExhibit:5629499534213120:5752754626625536"
  }) {
    checkedCircleExhibit {
      id
      circle {
        id
        # 削除前はここにデータが存在していた…！
        loginUserChecked {
          id
        }
      }
    }
  }
}
//}

//list[remove-checked-mutation-resp][消した箇所のデータがnullになっている]{
{
  "data": {
    "removeCheckedCircle": {
      "checkedCircleExhibit": {
        "id": "CheckedCircleExhibit:5629499534213120:5752754626625536",
        "circle": {
          "id": "CircleExhibitInfo:5752754626625536",
          "loginUserChecked": null
        }
      }
    }
  }
}
//}

すると、キャッシュ中のサークルと、サークルチェックのデータの繋がりが失われるため、自動的に画面は意図どおりに更新されます。
このように、Mutationの返り値はキャッシュデータを更新するためにある、と考えるとGitHub方式のほうがコード中でキャッシュ操作を行わなくてもよいためお手軽です。
