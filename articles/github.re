={github} GitHub v4 APIに倣え！

GitHubはGraphQLを使ったv4 APIを提供しています@<fn>{github-v4}。
GitHubはおそらく皆さん使っているでしょうから、GitHub自体の説明などは省いてよいでしょう。
どういうデータ構造があるのかも、ある程度想像できるのではないでしょうか？

//footnote[github-v4][@<href>{https://developer.github.com/v4/}]

GitHubのAPIはさすが数多の開発者が利用しているサービスだけあって、さまざまなところまでスキーマ設計上の配慮が行き届いています。
筆者が色々と試して経験を積んだ結果、GitHubのプラクティスを読み解き、自分のスキーマに反映させる価値は大いにあります。

この章では、GitHubがどのような工夫を行っているかを解説していきます。
本書ではGraphiQLなどPlaygroundの使い方はいちいち解説しませんが、Document Explorer（@<img>{document-explorer}）片手にみなさんも色々と探検してみてください。

//image[document-explorer][Document Explorer]{
//}

== Relayへの準拠

GitHub v4 APIはFacebookが開発するJS+React向けクライアントライブラリである"Relay"の定める仕様を踏襲しているところです。
Relayがどのような仕様を定めているかは@<chapref>{relay}で解説します。

概要だけ触れておくと、IDの設計方法、ページングについて、Mutationの設計について、の3点+αです。

GitHub v4 APIを丹念に調べることで、Relayの仕様をどのように組み込み、またそれを踏襲した上で拡張するかを学ぶことができます。
GraphQLスキーマは自由ですが、すべてを自分で考えて試行錯誤するよりも、既存の教えから学ぶのが楽です。

== ID設計論

これもRelayの仕様が絡んでいて、@<chapref>{relay}で具体的な解説をします。
ここでは、GitHib v4 APIから学ぶべきことにフォーカスして論じます。

それは、IDを人間が見て分かる形式にするか、それとも何らかの符号化をして隠すかです。
技術書典Webでは@<code>{"Event:tbf07"}のような、人が見て分かる（そしてプログラム上で簡単に合成できる）ルールを採用しています。
一方、GitHubでは@<code>{"MDEyOk9yZ2FuaXphdGlvbjEzNDIwMDQ="}のような形式です。
明らかにbase64なのでデコード可能ですが、ここでは気にしないことにします。

符号化することにより、次のような恩恵があると考えられます。

 1. IDの書式を自由に変更可能
 1. 書式を変更した上で、過去の形式を判別し内部変換可能
 1. 書式にユーザが依存しないようにする

突き詰めると、DB上のIDをサーバ上でしか復号できないように暗号化して露出させる、というのもありうるかというと、ありうるでしょう。
しかし、実際には既存のREST APIやURL設計との折り合いをつけるため、クライアント側でIDを合成可能にしておく余地はあります。

その意味では、GitHub v4 APIのIDもクライアント側で合成可能な形式であるとも考えられます。

どちらの方式もメリット・デメリットが存在するので、自分の運用するサービスではどちらのメリットが勝つかをよく考えて検討するとよいでしょう。

== 命名規則について

GraphQLには命名規則についてのガイドがありません。
なので、GitHubを参考にして考えてみます。

Queryの場合、repositoryやrepositoriesのような命名規則になっています。
素直にリソース名（名詞）とその複数形を使っています。

基本的にはこのプラクティスを踏襲すればよいでしょう。
筆者はrepositoriesのような複数形の代わりに、repositoryListのような命名規則も試してもみました。
しかし、返り値の型がリストではなくConnectionだったりするため、いまいちしっくり来なくて複数形派に鞍替えしました。

Mutationの場合、addStarやcreateIssueのように、"動詞+名詞"の形です。
面白いのは、insertという動詞を使っているMutationが存在していないところです。
MutationをDBへの操作の抽象と考えてしまうと、思わずinsertという動詞を使ってしまいそうです。
しかし、GitHub v4 APIではaddやcreateなどのメソッド名によく使われる動詞が選択されています。
つまり、MutationはDB操作の抽象ではなくて、ビジネスロジック、画面とのコミュニケーションツールとして設計されていることが汲み取れます。

ちなみに、deleteとremoveは両方存在します。
createとaddの両方が存在しているのと同じ理由でしょうかね。
英語の機微はわかりにくいのじゃよ。

Mutationについても、筆者は当初GraphiQLなどでの使い勝手を考慮し、"名詞+動詞"の形、たとえばissueAddを検討しました。
何かしたい時に、リソース名（名詞）はまぁだいたいのパターンで分かるだろうから入力補完が賢く働くように…という理由でした。
しかし、実際に試してみると@<img>{suggest}のようにprefix以外でもマッチすることがわかったため、GitHubと同じでいいか…となりました。
我々はツールの上で開発生活をしているので、ツール上で困らない、というのは重要な判断上の指針です。

//image[suggest][先頭以外でも普通にマッチする]{
issue って書いたら closeIssue とかがサジェストされる様子
//}

"動詞+名詞"パターン以外に、"[module][object][action]"という派閥@<fn>{another-way}も存在するようです。
しかし、Relayの例@<fn>{mutation-name-on-relay}やShopifyの例@<fn>{mutation-name-on-shopify}でも、GitHubと同じような規則を使っているようです。

//footnote[another-way][@<href>{http://blog.rstankov.com/how-product-hunt-structures-graphql-mutations/}]
//footnote[mutation-name-on-relay][@<href>{https://relay.dev/docs/en/mutations}]
//footnote[mutation-name-on-shopify][@<href>{https://github.com/Shopify/graphql-design-tutorial/}]

input型や、Mutationの返り値の型なども参考になります。
@<code>{addStar(input: AddStarInput!): AddStarPayload}や@<code>{transferIssue(input: TransferIssueInput!): TransferIssuePayload}などを見て分かるとおり、"Mutationの名前+Input"や"Mutationの名前+Payload"という規則を採用しているようです。

筆者はinput型の命名規則はGitHubを踏襲しつつ、返り値の型は@<code>{createCircle}や@<code>{updateCircle}で共通の@<code>{CirclePayload}に共通化しています。
今の所これで問題にはなっていません。
クエリ中に返り値の型は出てこないため、将来的に返り値の型を個別に変更しても大したBreaking Changeにはならなかろう…という慢心をしています@<fn>{its-flag}。

//footnote[its-flag][完全にフラグである]

== interfaceとunion typesとURL設計

GitHubにはなかなか長い歴史があります。
そのため、データ構造は非常に複雑です。
GraphQLにはinterfaceやunion typesといった概念があり、GitHub v4 APIはこれをかなり使い倒しています。
この節で言いたいことをざっくりまとめると、"複雑なデータ構造でもしっかりGraphQL上に表現できる"ということです。

interfaceは何らかの共通点がある要素を抽象化したものです。
多くのプログラミング言語にあるinterfaceとだいたい同じものです。
union typesはinterfaceと似た特徴がありますが、共通するフィールドを持たずクライアント側ではfragmentの併用が必須です。

たとえば、@<code>{User}型に着目してみます。

実装しているinterfaceは@<code>{Node}に始まり、@<code>{Actor}、@<code>{RegistryPackageOwner}、@<code>{RegistryPackageSearch}、@<code>{ProjectOwner}、@<code>{RepositoryOwner}、@<code>{UniformResourceLocatable}、@<code>{ProfileOwner}、@<code>{Sponsorable}があります。
複雑ですね…。
これはモデルの構造を反映していると考えられます。
何らかのデータのオーナーや、操作の主体になれる場合、interfaceが利用されていそうです。

また、@<code>{User}型が含まれるunion typesは@<code>{AuditEntryActor}、@<code>{Assignee}、@<code>{ReviewDismissalAllowanceActor}、@<code>{PushAllowanceActor}、@<code>{SearchResultItem}、@<code>{CollectionItemContent}のようです@<fn>{list-union-types}。
これまた複雑です。
何らかの操作の対象、またはログやデータの一部となる場合、union typesが利用されていそうです。

//footnote[list-union-types][interfaceと違い、どのunion typesに属しているか調べるのはDocument Explorerでは難しいので、Introspection Queryの結果から手で検索しました]

URLの構造に注目してみます。
@<href>{https://github.com/vvakame/metago}の場合、@<code>{vvakame}部分は@<code>{User}型の値です。
一方、@<code>{https://github.com/google/wire}の@<code>{google}部分は@<code>{Organization}型の値です。
ここが統一されていないのは人間にとってのわかりやすさのためでしょうし、歴史的経緯の面もあると思います。

リポジトリの情報を取得するQueryに着目すると@<code>{repository(owner: "google", name: "wire")}という引数になります。
ここでは、ownerの型は区別されませんし、GraphQL的な意味でのID@<fn>{google-id}を渡すこともできません。
人間の認識やUIに寄り添った設計になっています。
@<code>{ownerId}ではなく@<code>{owner}を引数に要求しているのは、何を渡してほしいのかを暗に伝える、よい設計といえます。

//footnote[google-id][Google orgのIDは@<code>{"MDEyOk9yZ2FuaXphdGlvbjEzNDIwMDQ="}で、databaseIdは@<code>{1342004}です。]

GitHubのAPIはidやdatabaseIdを引数に求める箇所と、前述のように人間が"ID的だ"と考えている名前を指定させる箇所に分かれています。
我々も、自分のスキーマでどのQueryやMutationが画面上でどのように使われうるのかを考え、引数に何を求めるかを考えるべきでしょう。

そのためには、一度システムを組んでみて試用してみる。
本格的なリリースの前にスキーマを再設計する工程を考慮に入れる、などの工夫が必要でしょう。
やる前に困る箇所をすべて潰せると幸せですが、使わずに困ることは難しいですからね。

== セキュリティのためのrate limitとcomplexity

GraphQLにもセキュリティの問題があります。
クライアント側でクエリを自由に組み立てることが可能なので、いくらでも巨大なリクエストを作れてしまうのです。

この問題に対応するため、GitHub v4 APIではresource limitations@<fn>{github-resource-limitations}が定められています。
制限には2種類あり、クエリ1回あたりの複雑度を定めるNode limitと、1時間あたりにどれだけのコストを消費できるかを定めるRate limitがあります。

//footnote[github-resource-limitations][@<href>{https://developer.github.com/v4/guides/resource-limitations/}]

詳しい解説は公式ドキュメントを参照してください。
かなりわかりやすく書かれています。
図をさらっと流し見するだけでも骨子を掴むには十分です。
また、この問題をどのように御するかは@<chapref>{relay}でも触れていきます。

外部にAPIを公開する場合、この2種類の考え方が必要になるでしょう。
もし、公式なクライアントしか存在せず、それ以外のリクエストを考慮しなくてよい場合でも、Node limitは設けたほうがよいでしょう。

具体的な対処として、クエリを実行する前にそのクエリがどのくらいのデータを要求しているかを見積もります。
この複雑度を見積もれるようにするためには、スキーマの設計が重要です。
あるフィールドがリストを返す場合、クエリの時点で何件要求しているか、引数に指定させる構造にする必要があります。
これにより、参照しているフィールドの数、リストの長さを勘案し、クエリを実行せずに複雑度を見積もれるようにするのです。

もう一度書きます。
複雑度を見積もれるようにするためには、スキーマの設計が重要です。
ここ重要なので覚えておいてください…。
たぶんこうしておけば将来的にコスト計算できるでしょ… と思って組んでおいても、後々実際に実装してみると漏れが見つかる場合があります。
制限をするかしないかはおいておいて、複雑度の計算処理（とそのロギング）自体はなるべく早い段階で組み込んでおくのをお勧めします@<fn>{admonish-myself-1}。

//footnote[admonish-myself-1][この本を書くために複雑度計算処理を実装してみたら諸々漏れてて今から修正するのめんどくさい…]

クエリの複雑度が高くなる主な要因は、一度に取得するデータ量、もっというと要求するリストの長さによって大きく左右されます。
複雑度30のノードを10件要求すると、複雑度は300になります。
しかし、複雑度10のノードを100件要求すると、複雑度は1000にもなります。
複雑度を低減するには、ノード単体の要求の多寡よりもリスト長に大きく左右されるわけです。

複雑度を見積もった後、クエリを実行すると実際のコストは遥かに安い場合というのは存在します。
たとえば、1000件要求したけど実際のデータは10件だった…という場合です。

ここで実際のコストは計算する必要はあまりありません。
慈悲の心を発揮したりはせず、見積もりのコストを元にバジェットから差っ引きます。

もし、実コストベースで運用してしまうと、将来的にデータ量が増えたとき、ある瞬間に今までは許されていたクエリが許されなくなってしまう可能性があるからです。
あと実装がめんどくさい。

制限が存在することにより、フロントエンドの開発者はクエリのコストを気にするようになります。
これはバックエンドの実装担当としては歓迎すべきことです。
システム上問題が発生せず、かつなるべく大きいバジェットを確保できるよう協力しあうとよいでしょう。

リスト長を短くするのは複雑度以外に、計算時間やデータ取得にかかる時間の軽減にも役に立ちます。
制限が許す範囲で妥当な範囲に短いリスト長を要求し、必要であればページングをうまく行い1クエリあたりの要求を減らす。
すると、ユーザに何らかのインタラクションを返すまでの時間も短くなる恩恵があります。

フロントエンドエンジニア的には面倒なのはわかりますが、データがたかだか640件くらいしか存在しないからといって、1000件とりあえず要求してみるのはやめましょう@<fn>{admonish-myself-2}。

//footnote[admonish-myself-2][すみませんでした…]

許すコストをどのように設定するか…というのは難しい問題です。
まずは画面上、もっとも複雑であろうもののクエリで試算&妥当か検討してみて、そこから導出するのがよいのではないかと考えています。

== Schema Changes

GitHub v4 APIはスキーマの変更をちゃんとトラッキングし、公開しています@<fn>{schema-changes}。
また、破壊的変更の予告も公開しています@<fn>{breaking-change}。

//footnote[schema-changes][@<href>{https://developer.github.com/v4/changelog/}]
//footnote[breaking-change][@<href>{https://developer.github.com/v4/breaking_changes/}]

偉いですね…。

GraphQL自体は@<code>{@deprecated}という組み込みのdirectiveを持っています。
これをちゃんと使ったほうがよい気はします。

筆者はまだ破壊的変更をしたら自分で関係箇所を直せばよいシステムしか運用していないため、このあたりにあまり知見がありません。
