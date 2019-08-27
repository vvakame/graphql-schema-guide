= Relay 各仕様解説

Relay@<fn>{relay}はFacebookが作成しているReact+GraphQLでフロントエンドの開発を行うためのライブラリです。
そして、Relayを便利に使うためのGraphQLスキーマに対する追加仕様がいくつか定義されています。
「私はApolloを使うから関係ないね！」という人にも便利な仕様ですので、ぜひ読んでもらいたいです。

//footnote[relay][@<href>{https://relay.dev/}]

@<chapref>{github}で解説しているように、GitHubですらRelayの追加仕様をほぼ踏襲しています。

Relayが求めるサーバ側（スキーマ）の仕様の狙いは次のとおりです。

 * オブジェクトを再取得するため
 * Connectionを通じてページングを実装するため
 * Mutationの結果を予測可能にするため

これを具体的な仕様として説明してみます。
TODO 消す

 * Cursor Connections（カーソルによるリストの連結）@<fn>{cursor-connections}
 ** ページネーションが必要な箇所はこの構造にするべし
 ** リストを返したい箇所で@<code>{Connection}サフィックスの型を用意する
 ** @<code>{first: Int!}と@<code>{after: String}を引数に設ける
 ** Connectionはcursorを持てる@<code>{Edge}と、ページング情報をもつ@<code>{PageInfo}をもつ
 * Input Object Mutations（@<code>{clientMutationId}をつけよう）@<fn>{input-object-mutations}
 ** Mutationのinputに@<code>{clientMutationId: String}をつけて、返り値にも同値をコピーする
 ** リクエストとレスポンスのひも付きがわかりやすいよ！
 ** わりと死んでる仕様感はある
 * Mutations updater（Mutationsで返すべき情報）
 ** 明確に仕様化はされていないがサーバ&クライアント協力して配慮すべき…
 ** 削除したデータのIDが分かるようにする
 ** データを追加した場合、どこかのConnectionに追加するべきか検討する

//footnote[cursor-connections][@<href>{https://relay.dev/graphql/connections.htm}]
//footnote[input-object-mutations][@<href>{https://relay.dev/graphql/mutations.htm}]
//footnote[client-mutation-id-is-dead][@<href>{https://github.com/facebook/relay/pull/2349}]

== Global Object Identification

Global Object Identification@<fn>{object-identification}は全データをIDにより一意に特定可能にし、再取得を自動的に可能にする仕様です。

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

これは結構厄介な問題で、マイクロサービス構成を取り背後に複数のサーバAPIがある場合、どのサービスのどのテーブルのどのIDのデータかをもIDに含めなければシステム全体で一意に特定できないでしょう。

GitHub v4 API@<fn>{github-explorer}を例に見てみましょう。
試しに、GoogleのorganizationのIDを見てみます（@<list>{code/relay/globalObjectIdentification/github-1.graphql}、@<list>{code/relay/globalObjectIdentification/github-1-result.json}）。
みると、@<code>{"MDEyOk9yZ2FuaXphdGlvbjEzNDIwMDQ="}という値ですね。
見るからにbase64っぽいので、デコードしてみると@<code>{012:Organization1342004}が得られます。
Organizationはデータ種別、テーブル名とおそらくイコールでしょう。
1342004はデータベースIDと一致しているのでデータのIDでしょう。
先頭の012の部分は不明ですが、おそらくIDの書式バージョンを表しているのではないかと筆者は考えています。

//footnote[github-explorer][@<href>{https://developer.github.com/v4/explorer/}]

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
技術書典Webでは人間が見て意味がわかりやすく、クライアント側で合成するのが容易です。
一方、将来的にIDの構造に破壊的変更を加えようとした時、困りそうだというのは予想がつきます。
GitHubのやり方は人間には若干不親切ですが、将来的な破壊的変更を内部で吸収する余地があります。

グローバルなIDがどうやって成り立っているかを見たので、次はQueryのnodeフィールド側です。
GitHub v4 APIの例を引き続き使って説明します。
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

この機能が実際に必要なのか？というと筆者がApolloユーザであることもあり、実用上役に立っているのを見かけたことはありません。
しかしながら、この機能は開発時にかなり役に立ちます。
得られたデータを元に同じデータが別の形でほしい場合、どのフィールドを使えば欲しいものが手に入るか？を考えるのは大変です。
もしかしたら、その手段がスキーマ上に存在していないことすらあるでしょう。
細かいことを気にしたり、不便さを感じるくらいであれば、ある程度定型的な実装を育てておけば需要を満たしてくれるnodeフィールドはリーズナブルな選択です。

Relayの定める仕様には存在しませんが、@<code>{nodes(ids: [ID!]!): [Node]!}というフィールドもついでに定義するとよいでしょう。
これがあれば、@<code>{node(id: ID!): Node}も実装を流用して組むことができるでしょう。

== Cursor Connections

TODO
スキーマの話
first, after
last, before

== Input Object Mutations

TODO
割と死んでる
inputをちゃんと使う（args並べて対処しない）とか、返り値で結果直接返すんじゃなくて1つラッパーとなる型を作れとかを守るのにはちょうどいいおまじないかもね

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

Mutationによるデータの新規作成・更新はレスポンスを見れば自動的にキャッシュ（≒画面）に反映できます。
一方、先の3種の操作はレスポンスを見ただけではキャッシュの状態を最適な状態に保つことはできません。
それぞれ、どう対応するべきかを見ていきます。

データを削除する場合、一般的なユースケースでは削除したいIDをリクエストに含めるでしょう。
そのためクライアント側はドメイン知識があれば、処理が成功した時点でどのIDをキャッシュから消せばよいか分かります。
しかしながら、複数のデータを消したり、削除対象のIDがサーバ側で決まるパターンもあるでしょう。
このようなパターンと一貫性のある設計とするため、レスポンスには必ず削除したIDを含めるようにします。

残念ながら、削除したIDであることを示すコンセンサスの取れたフィールド名はありません。
レスポンスに削除したIDを含むデータ全体を返すのでもよいですし、削除したIDをもつフィールドをPayloadに明示的に含めるのでもよいでしょう。
GitHub v4 APIは前者を選んでいます。

Connectionへの追加、削除についてはドメイン知識が必要になるため、ライブラリに全自動的に処理させるのは難しいでしょう。
代わりに、今行ったMutationがどういう処理で、どういう結果が返ってきて、どうやったら既存のConnectionに追加、削除できるかを人間は理解できるはずです。
人間が頑張りましょう。
そのためには、Connectionを取得する時に利用したFragmentをレスポンスに対して再利用できるような構造でなくてはいけません。
REST APIだとつい空のJSONを返してしまったりする場合がありますが、GraphQLではちゃんと操作した該当データを返却するようにしましょう。
