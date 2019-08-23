= 前書き

本書は筆者（@vvakame）がGraphQL@<fn>{graphql-url}を使った開発の中で得た知見を振り返り、解説するものです。
過去にGo言語+gqlgen@<fn>{gqlgen-web}という構成で、MTC2018カンファレンスLP@<fn>{mtc2018}、技術書典Web@<fn>{tbf-web}、弊社社内ツール@<fn>{internal-tool}を作成してきました。
ここでの経験値をここで活かしていくスタイル…！

//footnote[graphql-url][@<href>{https://graphql.org}]
#@# prh:disable:web
//footnote[gqlgen-web][@<href>{https://gqlgen.com/}]
//footnote[mtc2018][@<href>{https://tech.mercari.com/entry/2018/10/24/111227}]
#@# prh:disable:web
//footnote[tbf-web][@<href>{https://techbookfest.org/}]
//footnote[internal-tool][今のところ外部で詳細な話をしたことがない。Spanner向けサポートツール]

スキーマを中心に据えたベストプラクティスを書こうと思います。
なので、Go言語に限らず、そしてスキーマファースト・コードファーストに限らず、読み進められるものになる（はず）です。

GraphQLの基礎はスキーマであり、どう設計するかでクライアントがどの程度楽をできるか、サーバ側で制限をかけやすくなるかなどが決まります。
また、運用を開始した後だとスキーマを変えるのは他のプロトコルと同程度には気を使うと思いますので最初にそれなりに正しい設計を考えておきたいものです。
本書の主張をめちゃめちゃ端的に表すと 1. GitHub v4 APIに倣え！ 2. Relayの仕様を学べ！ の2点に集約されるといっても過言ではないでしょう。

本書の前身として、"GraphQLサーバをGo言語で作る"@<fn>{graphql-with-go-book}と"Apolloドハマリ事件簿"@<fn>{apollo-swamped-book}があります。
booth.pmで販売またはGitHubで全文を公開していますので、こちらもぜひ読んでみてください。

//footnote[graphql-with-go-book][@<href>{https://vvakame.booth.pm/items/1055228}]
//footnote[apollo-swamped-book][@<href>{https://vvakame.booth.pm/items/1321051}]

#@# TODO ○○章では○○を紹介します みたいな一覧あったほうがいいかも
#@# TODO ↓全体的にこの章じゃなくない？

== GraphQLの概要

GraphQLはGraph構造のデータから任意のデータを取り出すためのQuery Languageです。
GraphQLスキーマはデータの構造+データ間の繋がり+型によって定義されます。
もちろん、静的型付けなので実行前にクエリが正しいか、フロントエンドのコンポーネント内でデータの形式が噛み合っているかをチェックさせることができます。
最高ですね！

SQLとGraphQLはDBへのクエリとサーバAPIへのクエリという点でだいぶ隔たりがありますが、比較してみます。

SQLは@<kw>{RDB,Relation Database}に対してデータを要求する時に使います。
結果は基本的に表形式、つまり2次元のデータとして得られます。

一方GraphQLはサーバAPIに対してデータを要求する時に使います。
結果はツリー構造で得られます。

#@# TODO SQLで得られる結果とGraphQLで得られる結果の模式図を書く

得られる形式に注目した場合、データを使った計算処理やバッチ処理を行うには2次元のデータのほうが都合がよく、画面で利用するにはツリー構造のほうが都合がよいでしょう。

なぜそういう違いになるかというと、データの結合に対する考え方が異なるのです。
RDBではテーブルに外部キーとなるカラムを持ち、クエリでもって何をどう結合するかを決定します。

対してGraphQLではスキーマの段階でどのタイプ（RDBでいうテーブル）がどのタイプに広がっていくか（関連しているか）を定義します。
GraphQLでのクエリはそのスキーマに対して"こういうレイアウトでデータをおくれ"と頼むだけで、結合の仕方を変更する能力はありません。
スキーマ上での関係性の定義は柔軟です。
たとえば@<code>{User}型に@<code>{Friends(first: Int, after: String): [User!]}というフィールドがあるとします。
すると、クエリでは"User"の"Friends"の"Friends"をくれ！という具合に定義された関係性を好きな深さまで辿っていくことができます@<fn>{not-infinit}。

//footnote[not-infinit][もちろん現実的にはDBの負荷や計算量やレスポンスにかかる時間を考えどこかで制限をかける必要があります。RDBと同じ。]

もちろん、クライアントからクエリを投げられるということは、SQLのように発行者が常に信用できるとは限らないため、セキュリティについて考える必要があります。
事前に危険なクエリではないか調査可能にするためには、スキーマ設計の段階で気をつけるべき問題があります。

#@# TODO 章リンク的なの貼る

== どうやってGraphQLサーバは動作するのか

GraphQLサーバはresolverによってクエリを結果のJSONに変換します。
どんな形のクエリでも一発で目的のJSONに解決（resolve）できるResolverを想像すると、とんでもなく複雑です。
普通に考えて、どういう設計にすればいいのかすら検討がつかないでしょう。

しかし、@<code>{UserID}を@<code>{User}に変換するresolverであれば実装は容易です。
さらに@<code>{User}から@<code>{FriendIDs}を取得することはできそうですし、そうしたら先のresolverを使って@<code>{User}の配列にすることもできそうです。
このように、スキーマのパーツからパーツへの変換を行う無数のresolverを作成し、それを組み合わせ合体させたものが、当初欲しかったクエリのresolverとなるのです。
あとは、実際のクエリにあわせて必要なresolver@<strong>{のみ}を動作させて結果を作成します。

ものすごく無駄かつ豪華な実装では、GraphQLスキーマのすべてのフィールドに対して1つの1つresolverを作ることも可能です。
ですが@<code>{User.id}、@<code>{User.name}、@<code>{User.email}のような、1つのテーブルから一発で取得できるようなフィールドに個別にresolverを実装していてはストレスで胃に穴が3個くらい開いてしまいます。
なので、1つの型に対して最低1つのresolverを作り、割り当てるようにするのが一般的です。
もちろん@<code>{User.CPU使って計算するフィールド}のような、本来DB上に存在しないものはresolverの実装を与える必要があります。
よくあるパターンとしては@<code>{User.GroupID}のような別データのIDをもつ場合は@<code>{User.Group}に見せかけるために@<code>{GroupID}から@<code>{Group}に変換するようなresolverが必要になります。

GraphQLのスキーマとDB上のデータをつなぎ合わせるためには、resolverをどう実装するかが重要になるわけです。


//comment{
 * 技術書典で得た知見の話
 ** 色々ありすぎるのでは？
 * graphql-schema-linter用のルールを自分で書く話
 * GitHub v4 APIをあがめよ
 ** GitHubがやってないことはやらない
 ** unionとかの実例
 * Relay系の仕様の解説
 ** https://relay.dev/docs/en/mutations#range_add とかも
 * Resolverの概念の説明
 ** スキーマのモデルとコード上のモデルの対応と意図的な乖離の話
 * 画面の都合とGraphQL
 ** Queryの話(クライアント気にしなくていい)
 ** Mutationの話(クライアント気にしたほうがいい)
 * グラフの話
 ** グラフ構造を作らないと楽にならない
 ** resolverを使い倒す必要がある
 * データ取得のバッチ化の話
 ** 過去にgqlgenの話で書いたことあった気がするなー再録かなー
 * RDBの話
 ** 難しいね実際
 * directiveの話
 * postmanの話？
 ** https://twitter.com/vvakame/status/1153654008952659969
 * ロギングの話
 * 誰にGraphQLをオススメするか？という話
 * DBのスキーマとGraphQLのスキーマ
 ** なるべく対応してたほうが楽という話
 ** 逆に乖離させる理由ある？
 * ツリー構造が勝利の鍵！
 ** 正規化って要するにツリー構造だよね
 * SQL vs GraphQL
 ** SQLは結果を2次元に落とさないといけない
 * 誰のfieldになるかで検索条件の指定が絞れる話
 * コードの実装の整理の仕方の話
 ** ドメイン層とGraphQL層を分けるとかそういう
 * どういう時にエラーが発生することが許されるか
 ** 基本的にはQueryではエラーにしない
 ** 検索条件によるエラーは許容してもいいかも… でもドキュメントに書いたほうがよいしなるべくデフォルト値を適用してあげたい
 * gRPCとGraphQL
 * https://tech.mercari.com/entry/2018/10/24/111227 あたり読み返す？
 * https://github.com/vvakame/graphql-with-go-book に何書いたかも忘れた
 * 命名規則の話
 ** [action][module][object] とか
 * エラーハンドリングパターン
 ** いまんとこ妙案なし
 * schemaでざっくりしたtype毎に定義を分割する話
 ** Mutationを画面フレンドリーにしようとして爆発四散する話
 * コードファーストの罠
 ** IntrospectionQueryではdirective取れないので注意という話
//}
