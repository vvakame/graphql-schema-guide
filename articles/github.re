={github} GitHub v4 APIに倣え！

この章では、GitHubがどのような工夫を行っているかを解説していきます。
本書ではGraphiQLなどPlaygroundの使い方はいちいち解説しません。
が、Document Explorer（@<img>{document-explorer}）片手にみなさんも色々と探検してみてください。

//image[document-explorer][Document Explorer][scale=0.4]{
//}

GitHubはGraphQLを使ったv4 APIを提供しています@<fn>{github-v4}。
GitHubはおそらく皆さん使っているでしょうから、GitHub自体の説明は省いてよいでしょう。

//footnote[github-v4][@<href>{https://docs.github.com/en/free-pro-team@latest/graphql}]

GitHubのAPIはさすが数多の開発者が利用しているサービスだけあって、さまざまなところまでスキーマ設計上の配慮が行き届いています。
筆者が色々と試して経験を積んだ結果、GitHubから得られるプラクティスには大きな価値があると感じるようになりました。

最近のGitHubはドキュメンテーションシステムを刷新しつつあり、第1版執筆時点とだいぶドキュメントの構成が様変わりしました。
第1版からの読者さんも、ReferenceのQuerirs, Mutations, Objects, ... と各項目を眺めなおすと、新しい発見があるかもしれません。

== Relay向けサーバ仕様への準拠

GitHub v4 APIはFacebookが開発するJS+React向けクライアントライブラリである"Relay"が要求するサーバ仕様@<fn>{graphql-server-spec}を踏襲しています。
GitHub v4 APIを丹念に調べることで、Relay向けの仕様をどのように組み込み、またそれを踏襲した上で拡張するかを学ぶことができます。
GraphQLスキーマは自由ですが、すべてを自分で考えて試行錯誤するよりも、既存の教えから学ぶのが楽です。
Relayがどのような仕様を要求しているかは@<chapref>{relay}で解説します。
#@# OK gfx: なんでいちクライアントライブラリの話が急にくるのか混乱のもとなのでもっと説明がほしい。クライアントライブラリのRelayが定めるGraphQL API仕様のこともRelayと呼ぶよ！この本でRelayといったら次からは「Relayが定めるGraphQL API仕様」のことだよ！とか書いたほうがいいと思う。（昔は "Relay Server Specification" という名前だったけど今は "GraphQL Server Specification" と呼ばれていて言及しにくい https://relay.dev/docs/en/graphql-server-specification ）
#@# OK vv: たしかに

//footnote[graphql-server-spec][@<href>{https://relay.dev/docs/en/graphql-server-specification}]

== ID設計論

これもRelayのサーバ仕様が絡んでいて、@<chapref>{relay}で具体的な解説をします。
ここでは、GitHib v4 APIから学ぶべきことにフォーカスして論じます。

IDの形式を人間が見て分かるものにするか、それとも何らかの符号化をして隠すか。
技術書典Webでは@<code>{"Event:tbf07"}のような、人が見て分かる（そしてプログラム上で簡単に生成できる）ルールを採用しています。
一方、GitHubでは@<code>{"MDEyOk9yZ2FuaXphdGlvbjEzNDIwMDQ="}のような形式です。
#@# OK sonatard:「合成できる」は、IDをクライアントサイドで生成できるという意味であってますか？
#@# OK vv: あってます！生成できる に置き換えてしまうことにします

符号化することにより、次のような恩恵があると考えられます。

 1. IDの書式を自由に変更可能
 1. 書式バージョンを埋め込み、過去の形式を判別し内部で変換可能にする
 1. 書式にユーザが依存しないようにする

突き詰めると、DB上のIDをサーバ上でしか復号できないように暗号化して露出させる、というのもありえます。
しかし、実際には既存のREST APIやURL設計との折り合いをつけるため、クライアント側でIDを生成可能にしておくほうが圧倒的に楽です。
#@# OK gfx: KibelaではGitHubを参考にIDを設計して、クライアントでは合成しないようにした。でもいろいろ面倒だったな…。最終的にはIDからリソースを得るフィールドとは別に "noteFromPath(path: String!)" みたいな「パスを渡すとリソースを得る」みたいなフィールドを増やすことにした。ただまあ、public web api だからいま考え直してもこれにするかなと思う。とはいえ内部でしか使わないAPIならクライアントで合成できるようなフォーマットにしてもいいと思う。
#@# OK vv: 色々な判断がありそう ここはこのままにしておきます

その意味では、GitHub v4 APIのIDも、見るからにbase64なのでクライアント側で生成可能な形式であると考えられます@<fn>{github-id}。

//footnote[github-id][現実問題、GitHubはGraphQLのIDがわからなくても使えるようにスキーマを設計することで解決している気もします]

どちらの方式もメリット・デメリットが存在するので、自分の運用するサービスではどちらのメリットが勝つかをよく検討するとよいでしょう。
第二版執筆時点では、クライアント側でIDを合成可能にしたことをまだ後悔していません@<fn>{wanna-use-nominal-typing}。

//footnote[wanna-use-nominal-typing][IDごとにnominal typeを割り当てたいなと思うことは多々ある]

== 命名規則について

GraphQLには命名規則についてのガイドがありません。
なので、GitHubを参考にしてみます。

Queryの場合、@<code>{repository}や@<code>{repositories}のような命名規則になっています。
素直にリソース名（名詞）とその複数形を使っています。

基本方針としてこのプラクティスを踏襲します。
筆者は@<code>{repositories}のような複数形の代わりに、@<code>{repositoryList}のような命名規則も試しました。
しかし、返り値の型はRelayの仕様に従うとConnectionになるので、リストじゃないじゃん！となってしまい、いまいちしっくり来なくて複数形派に鞍替えしました。

Mutationの場合、@<code>{addStar}や@<code>{createIssue}のように、"動詞+名詞"の形です。
面白いのは、insertという動詞を使っているMutationが存在していないところです。
MutationをDBへの操作の抽象と考えてしまうと、思わずinsertという動詞を使ってしまいそうです。
しかし、GitHub v4 APIではaddやcreateなどの動詞が選択されています。
つまり、MutationはDB操作の抽象ではなくて、ビジネスロジック、画面とのコミュニケーションツールとして設計されていることが汲み取れます。

ちなみに、deleteとremoveは両方存在します。
createとaddの両方が存在しているのと同じ理由でしょうかね。
英語の機微はわかりにくいのじゃよ。

Mutationについても、筆者は当初GraphiQLなどでの使い勝手を考慮し、"名詞+動詞"の形、たとえば@<code>{issueAdd}のスタイルを検討しました。
何かしたい時に、リソース名（名詞）はまぁだいたいのパターンで分かるだろうから入力補完が賢く働くように…という理由でした。
しかし、実際に試してみると@<img>{suggest}のようにprefix以外でもマッチすることがわかったため、GitHubと同じでいいか…となりました。
我々はツールの上で開発生活をしているので、ツール上で困らない、というのは重要な判断上の指針です。
これも、第二版執筆時点で特に問題を感じていません。

//image[suggest][先頭以外でも普通にマッチする][scale=0.6]{
issue って書いたら closeIssue とかがサジェストされる様子
//}

"動詞+名詞"パターン以外に、"[module][object][action]"という派閥@<fn>{another-way}も存在するようです。
しかし、Relayの例@<fn>{mutation-name-on-relay}やShopifyの例@<fn>{mutation-name-on-shopify}でも、GitHubと同じような規則を使っています。

//footnote[another-way][@<href>{http://blog.rstankov.com/how-product-hunt-structures-graphql-mutations/}]
//footnote[mutation-name-on-relay][@<href>{https://relay.dev/docs/en/mutations}]
//footnote[mutation-name-on-shopify][@<href>{https://github.com/Shopify/graphql-design-tutorial/}]

GitHubの読み解きに戻ります。
input objectや、Mutationの返り値の型なども参考になります。
@<code>{addStar(input: AddStarInput!): AddStarPayload}や@<code>{transferIssue(input: TransferIssueInput!): TransferIssuePayload}などを見て分かるとおり、"Mutationの名前+Input"や"Mutationの名前+Payload"という規則を採用しているようです。

筆者はinput型の命名規則はGitHubを踏襲しつつ、返り値の型は@<code>{createCircle}や@<code>{updateCircle}で共通の@<code>{CirclePayload}に共通してしまうことが多いです。
クエリ中に返り値の型は出てこないため、将来的に返り値の型を個別に変更しても大したBreaking Changeにはならなかろう…という慢心をしています@<fn>{its-flag}。
第二版執筆時点で特に問題を感じていません。
とはいえ、必要であれば@<code>{PayPalCheckoutPayload}のような特定のMutation固有の型も作っています。

//footnote[its-flag][完全にフラグである]

== interfaceとunion typesとURL設計

GitHubにはなかなか長い歴史があります。
そのため、データ構造は非常に複雑です。
GraphQLにはinterfaceやunion typesといった概念があり、GitHub v4 APIはこれをかなり使い倒しています。
この節で言いたいことをざっくりまとめると、"複雑なデータ構造でもしっかりGraphQL上に表現できる"ということです。

interfaceは何らかの共通点がある要素を抽象化したものです。
多くのプログラミング言語にあるinterfaceとだいたい同じものです。

union typesはinterfaceと似た特徴がありますが、共通要素のない色々な型を返せるため、各型毎のフィールドの参照にはfragmentの利用が必須です。
#@# OK sonatard:「共通するフィールドを持たない」のイメージが難しいと思いました。色々な型を返せるの方がわかりやすい？

たとえば、@<code>{User}型@<fn>{user-object}に着目してみます。

//footnote[user-object][@<href>{https://docs.github.com/en/free-pro-team@latest/graphql/reference/objects#user}]

実装しているinterfaceは@<code>{Node}に始まり、
@<code>{Actor}、
@<code>{PackageOwner}、
@<code>{ProfileOwner}、
@<code>{ProjectOwner}、
@<code>{RepositoryOwner}、
@<code>{Sponsorable}、
@<code>{UniformResourceLocatable}
があります@<fn>{r1-diff-interface}。
複雑ですね…。
これはモデルの構造を反映していると考えられます。
何らかのデータのオーナーや、操作の主体になれる場合、interfaceが利用されていそうです。

//footnote[r1-diff-interface][第1版時点と比べて@<code>{PackageOwner}が追加、@<code>{RegistryPackageOwner}、@<code>{RegistryPackageSearch}が削除されている]

また、@<code>{User}型が含まれるunion typesは
@<code>{AuditEntryActor}、
@<code>{Assignee}、
@<code>{ReviewDismissalAllowanceActor}、
@<code>{PushAllowanceActor}、
@<code>{SearchResultItem}、
@<code>{EnterpriseMember}、
@<code>{RequestedReviewer}、
@<code>{Sponsor}
のようです@<fn>{list-union-types}@<fn>{r1-diff-union}。
これまた複雑です。
何らかの操作の対象、またはログやデータの一部となる場合、union typesが利用されていそうです。

//footnote[list-union-types][interfaceと違い、どのunion typesに属しているか調べるのはDocument Explorerでは難しいので、Introspection Queryの結果から手で検索しました]
//footnote[r1-diff-union][第1版時点と比べて@<code>{CollectionItemContent}が削除されている]

URLの構造に注目してみます。
@<href>{https://github.com/vvakame/metago}の場合、@<code>{vvakame}部分は@<code>{User}型の値です。
一方、@<code>{https://github.com/google/wire}の@<code>{google}部分は@<code>{Organization}型の値です。
それぞれ、@<code>{vvakame}や@<code>{google}の部分は文脈によって型が違いますが、両方とも@<code>{RepositoryOwner}を実装しています。

URLの書式が同じなのに、場合によって実際の型が異なるというのは人間にとってのわかりやすさのためでしょうし、歴史的経緯の面もあると思います。
そこを埋めるために@<code>{interface}が使われているわけです。

実際に、APIを叩いて両リポジトリのownerを調べてみます。
@<list>{repository-owner}のクエリを投げると、UserとOrganizationがそれぞれ返ってきていることがわかります（@<list>{repository-owner-result}）。
#@# OK sonatard:帰ってくるレスポンスが具体的に書いてあるとまったく違う型を返せるということがわかりやすいかなと思いました

//list[repository-owner][ownerの具体的な型が異なる]{
{
  a: repository(owner: "vvakame", name: "metago") {
    owner { # owner = ResourceOwner
      id
      ... on User {
        __typename
        name
        bio # Userにしか存在しない
      }
    }
  }
  b: repository(owner: "google", name: "wire") {
    owner { # owner = ResourceOwner
      id
      ... on Organization {
        __typename
        name
        teamsUrl # Organizationにしか存在しない
      }
    }
  }
}
//}

//list[repository-owner-result][結果]{
{
  "data": {
    "a": {
      "owner": {
        "id": "MDQ6VXNlcjEyNTMzMg==",
        "__typename": "User",
        "name": "Masahiro Wakame",
        "bio": "Love TypeScript, Go, GraphQL and Cat"
      }
    },
    "b": {
      "owner": {
        "id": "MDEyOk9yZ2FuaXphdGlvbjEzNDIwMDQ=",
        "__typename": "Organization",
        "name": "Google",
        "teamsUrl": "https://github.com/orgs/google/teams"
      }
    }
  }
}
//}

リポジトリの情報を取得するQueryに着目すると@<code>{repository(owner: "google", name: "wire")}という引数になります。
ここでは、ownerの型は区別されませんし、GraphQL的な意味でのID@<fn>{google-id}@<fn>{github-node-id}を渡すこともできません。
人間の認識やUIに寄り添った設計になっています。
@<code>{ownerId}と@<code>{owner}を明確に使い分けているのは、引数に何を渡してほしいのかを暗に伝える、よい設計といえます。

//footnote[google-id][Google orgのIDは@<code>{"MDEyOk9yZ2FuaXphdGlvbjEzNDIwMDQ="}で、databaseIdは@<code>{1342004}です。]
//footnote[github-node-id][横道 @<href>{https://docs.github.com/en/free-pro-team@latest/graphql/guides/using-global-node-ids} REST APIでもGraphQLでのIDである@<code>{node_id}を返すようになったらしい]

GitHubのAPIはidやdatabaseIdを引数に求める箇所と、前述のように人間が"ID的だ"と考えている名前を指定させる箇所に分かれています。
我々も自分のスキーマでどのQueryやMutationが画面上でどのように使われうるのかを考え、引数に何を求めるかを考えるべきでしょう。

そのためには、一度システムを組んでみて試用してみる。
本格的なリリースの前にスキーマを再設計する工程を考慮に入れる、などの工夫が必要でしょう。
使う前に困る箇所をすべて潰せると幸せですが、使わずに困ることは難しいですからね。

== enumの利用

GitHubのAPIは実に多数のenumを利用@<fn>{github-enum}しています。

//footnote[github-enum][@<href>{https://docs.github.com/en/free-pro-team@latest/graphql/reference/enums}]

第一版執筆時点では筆者はenumをほぼ使わずにシステムを構成していました。
しかし、後々enumは可能な限り使い倒す方がよい…と考えを改め、現在（第二版執筆時）に至っています。

enumを使う利点は、変更に強いことです。
"変更に強い"というのが何を指すかというと、何かを変えたときに影響範囲を適切に絞り込むことができ、破壊的変更を加えたら静的にそれがチェックできることを指すことにします。
enumの代わりに@<code>{String}を使っていると、その"値"がどういう性質かを保証することは難しくなります。
たとえば、Pull Requestの状態を表す@<code>{PullRequestState}の値を、Issueの状態を表す@<code>{IssueState}の値として流用できてしまったら困るわけです。
@<code>{CLOSED}を渡したら偶然動くかもしれないけど、@<code>{MERGED}を渡したらエラーになったりするわけです。
これの区別を行うためには型によるサポートが必要です。

フロントエンド側開発での仕様の把握が容易であるという点でも、enumの利用は好ましいです。
取りうる値は何か？前回確認時から選択肢は増えているのか？減った場合は何が減ったのか？という把握も楽になります。
スカラ型（Stringなど）の値やドキュメントの併用による運用より、機械的に保証できるほうが優秀なのはいうまでもないでしょう。

また、フロントエンド側からの入力をバックエンド側のGraphQLのフレームワークレベルで検知しエラーにできるのも得難い利点です。
フロントエンドからバックエンドまで一貫性のある仕組みによる担保は安心感が違います。

さて、第一版執筆時点でなぜenumを使わなかったのか？という点に少し触れておきます。
これは単にバックエンド側の構成に可能な限り手を加えたくなかったからです。
REST APIと構成をほぼ変えずに…という意図だったのですが、実際にenumを使うようにして振り返ると、足りなかったのは工夫でした。

既存の問題点は、1つの値が3種類の用途に使われていたことでした。

 1. 既存のAPI（REST API）としての露出
 2. GraphQL上への露出
 3. データベースへ保存するときの表現

これら3つすべてを@<code>{string}で実装している場合、それぞれに差をつけることはできません。
たとえば、いままで@<code>{"market"}という文字列を値に使っていた場合、GraphQLでの表現をenumで@<code>{"MARKET"}と変えたい場合、1も3も一緒くたに表現が変わってしまいます。

そこで、バックエンド上でも個別に型を割り当てるようにして、1、2、3のそれぞれの露出に対して変換ロジックを噛ませることができれば、用途別に異なる表現を取ることができます。
これを達成するために、型と値と用途ごとの変換ロジックを持ったコードをyamlから自動生成するツールを作りました。
わりとサクッと作れたので最初からやっておけばよかった…となりました。

みなさんも自分のアプリケーションでGraphQL関連の仕組みを作るときに、色々な値がどういう用途をもつのか、用途別の対応方法が用意できるかどうか、できないのであれば、どこを不動点にするべきかをはやめに検討するとよいでしょう。

== GitHubがやっていないこと

GitHubがやっていない、超絶テクニックを思いついてしまった場合、それはおそらくやらないほうがよいでしょう。

筆者が第一版の時点でやってみて、現時点ではやらないほうがよかった！と反省しているのは@<code>{Query}でのinput objectの利用です。
これについては@<chapref>{relay}で詳述しますが、GitHubがやっていないことです。
複数の選択肢が思いつくとき、GitHubが何をやっていないかに注目し考えてみるとよいでしょう。
なぜならば、それはGitHubもやろうと思ったけれどもメリットをデメリットが上回ったのでやらなかった、と考えられるからです。

新しい知見を得るには時に冒険も必要でしょうが、引き返せるタイミングを見極めながら挑戦したいものです。

== セキュリティのためのrate limitとcomplexity

GraphQLにもセキュリティの問題があります。
クライアント側でクエリを自由に組み立てることが可能なので、いくらでも巨大なリクエストを作れてしまうのです。

この問題に対応するため、GitHub v4 APIではresource limitations@<fn>{github-resource-limitations}が定められています。
制限には2種類あり、クエリ1回あたりの複雑度を定めるNode limitと、1時間あたりにどれだけのコストを消費できるかを定めるRate limitがあります。

//footnote[github-resource-limitations][@<href>{https://docs.github.com/en/free-pro-team@latest/graphql/overview/resource-limitations}]

詳しい解説は公式ドキュメントを参照してください。
かなりわかりやすく書かれています。
図をさらっと流し見するだけでも骨子を掴むには十分です。
また、この問題をどのように御するかは@<chapref>{relay}でも触れていきます。

外部にAPIを公開する場合、この2種類の考え方を真似する必要があります。
もし、公式なクライアントしか存在せず、それ以外のリクエストを考慮しなくてよい場合でも、Node limitは設けたほうがよいでしょう。
@<kw>{複雑度,complexity}計算の実装方法はフレームワークごとに異なりますので、各自調べてみてください。
#@# OK sonatard: 複雑度の計算の実装は自分でしなければいけないということがわかると読みやすいかと思いました

具体的な対処として、クエリを実行する前にそのクエリがどのくらいのデータを要求しているかを見積もります。
この複雑度を見積もれるようにするためには、スキーマの構造が重要です。

フィールドがリストを返す場合、フィールドの引数を見て最大何件が取得されるのかを計算できる必要があります。
たとえばユーザ10人について、それぞれのリポジトリを10件取得しようとした場合、最大で10+10×10＝110件のデータが取得される可能性があります。
このように、参照しているフィールドの数、リストの長さを計算すると、クエリを実行する前に複雑度を見積もれます。

筆者の経験談として、こうしておけば将来的にコスト計算できるでしょ… と思って組んでおいても、後々実際にコスト計算を実装してみると漏れが見つかる場合があります。
制限をするかしないかはさておいて、複雑度の計算処理（とそのロギング）自体はなるべく早い段階で組み込んでおくのをお勧めします@<fn>{admonish-myself-1}。

//footnote[admonish-myself-1][この本を書くために複雑度計算処理を実装してみたら諸々漏れてて今から修正するのめんどくさい…]

クエリの複雑度が高くなる主な要因は、一度に取得するデータ量、もっというと要求するリストの長さによって大きく左右されます。
複雑度30のノードを10件要求すると、複雑度は300になります。
しかし、複雑度10のノードを100件要求すると、複雑度は1000にもなります。
複雑度を低減するには、ノード単体の要求の多寡よりもリスト長に大きく左右されるわけです。

クエリを実行すると、事前の見積もりにくらべて実際のコストは遥かに安い場合があります。
たとえば、1000件要求したけど実際のデータは10件だった…という場合です。

ここで実際のコストを計算する必要はありません。
慈悲の心を発揮したりはせず、見積もりのコストを元にバジェットから差っ引きます。

もし、実コストベースで運用してしまうと将来的にデータ量が増えたとき、ある瞬間に今までは許されていたクエリが許されなくなってしまう可能性があるからです。
あと実コスト計算の実装をするのがめんどくさい。

制限が存在することにより、フロントエンドの開発者はクエリのコストを気にするようになります。
これはバックエンドの実装担当としては歓迎すべきことです。
システム上問題が発生せず、かつなるべく大きいバジェットを確保できるよう、許容する範囲を相談して決めるとよいでしょう。

リスト長を短くするのは複雑度以外に、計算時間やデータ取得にかかる時間の軽減にも役に立ちます。
制限が許す範囲で妥当な範囲に短いリスト長を要求し、必要であればページングをうまく行い1クエリあたりの要求を減らす。
すると、ユーザに何らかのインタラクションを返すまでの時間も短くなる恩恵があります。

フロントエンドエンジニア的には面倒なのはわかりますが、データがたかだか640件くらいしか存在しないからといって、1000件とりあえず要求してみるのはやめましょう@<fn>{admonish-myself-2}。

//footnote[admonish-myself-2][すみませんでした…]

許すコストをどのように設定するか…というのは難しい問題です。
まずは画面上、もっとも複雑であろうもののクエリで試算&妥当か検討してみて、そこから導出するのがよいのではないかと考えています。
#@# OK gfx: Kibelaでは内部APIは無制限に使えて公開APIに厳密にコスト/バジェットを設定するというダブスタな設計にしてしまった。しょうがないのじゃよ…。ただ内部といえどもAPIを叩くのが別チームならちゃんとやったほうがいい。
#@# vv: ﾜｶﾙｰ 今技術書典だとユーザの権限見てちょいちょい変えてるんだけどcomplexityの計算ちゃんとできるように改修して統一基準にしたい

ガッチガチに固めたい場合、Apolloが@<kw>{APQ,Automatic Persisted Queries}@<fn>{aqp}という仕様を整えています。
これは、クエリ全体を毎回送ったりパースしたりしないでよいように、ダイジェストだけを送りサーバ側がそれをキャッシュに持っていたらそれを使う、という仕組みです。
この仕組をさらに積極的に使うと、APIサーバをビルドするときに利用可能なクエリを事前登録しておき、そこにないリクエストはすべて弾く、とすることもできるでしょう。
そこまでガチガチの運用が必要なパターンはめったにないと思いますが…。

//footnote[aqp][@<href>{https://www.apollographql.com/docs/apollo-server/performance/apq/}]

== Schema Changes

GitHub v4 APIはスキーマの変更をちゃんとトラッキングし、公開しています@<fn>{schema-changes}。
また、破壊的変更の予告も公開しています@<fn>{breaking-change}。

//footnote[schema-changes][@<href>{https://developer.github.com/v4/changelog/}]
//footnote[breaking-change][@<href>{https://developer.github.com/v4/breaking_changes/}]

偉いですね…。

GraphQL自体は@<code>{@deprecated}という組み込みのdirectiveを持っています。
これはちゃんと使い倒していきましょう。

この問題については@<chapref>{tips}の@<hd>{tips|deprecated}でもう少し詳しく言及します。
