= 前書き

本書は筆者（@vvakame）がGraphQL@<fn>{graphql-url}を使った開発の中で得た知見を振り返り、解説するものです。
過去にGo言語+gqlgen@<fn>{gqlgen-web}という構成で、MTC2018カンファレンスLP@<fn>{mtc2018}、技術書典Web@<fn>{tbf-web}、弊社社内ツール@<fn>{internal-tool}を作成してきました。
それらで得た知見をこの本で解説します。

//footnote[graphql-url][@<href>{https://graphql.org/}]
#@# prh:disable:web
//footnote[gqlgen-web][@<href>{https://gqlgen.com/}]
//footnote[mtc2018][@<href>{https://tech.mercari.com/entry/2018/10/24/111227}]
#@# prh:disable:web
//footnote[tbf-web][@<href>{https://techbookfest.org/}]
//footnote[internal-tool][今のところ外部で詳細な話をしたことがない。Spanner向けサポートツール]

主にスキーマを中心に据えたベストプラクティスを紹介します。
Go言語に限らず、そしてスキーマファースト・コードファーストに限らず、読んで得るものがあるでしょう。

本書の前身として、"GraphQLサーバをGo言語で作る"@<fn>{graphql-with-go-book}と"Apolloドハマリ事件簿"@<fn>{apollo-swamped-book}があります。
booth.pmで販売またはGitHubで全文を公開していますので、こちらもぜひ読んでみてください。
特に、"GraphQLサーバをGo言語で作る"とは解説が重複している箇所もあります。

//footnote[graphql-with-go-book][@<href>{https://vvakame.booth.pm/items/1055228}]
//footnote[apollo-swamped-book][@<href>{https://vvakame.booth.pm/items/1321051}]

GraphQLの基礎はスキーマであり、設計次第でクライアントが楽をできるか、サーバ側で制約をかけやすくなるかなどが決まります。
また、運用を開始した後だとスキーマを変えるのは他のプロトコルと同程度には気を使うと思いますので最初にそれなりに正しい設計を考えておきたいものです。

本書の主張をめちゃめちゃ端的に表すと 1. GitHub v4 APIに倣え！ 2. Relayの仕様を学べ！ の2点に集約されるといっても過言ではありません。

お断りとして、筆者は普段MySQLなどに代表される@<kw>{RDB,Relational Database}を使っていません。
本書ではRDBが背景にある場合についても言及しますが、実践に裏打ちされたものではないため不足がないかどうかはわかりません。
筆者としてはGoogle Cloud DatastoreやGoogle Cloud Spannerなどの分散データベースの利用をお勧めしています。
AWSは知らん！たぶんAuroraあたりがそれなんじゃないでしょうか。

== 本書概要

@<chapref>{github}ではGraphQLの先駆者かつ圧倒的ユーザ支持のあるGitHubを例に、どのような設計上の工夫が行われているかを読み解きます。

#@# prh:disable:従って
@<chapref>{relay}ではGitHubも従っているRelayの仕様について解説します。Replayの仕様を学ぶことで、クライアントの都合のいいスキーマ設計のポイントを学ぶことができます。

@<chapref>{database}ではデータベースのスキーマとGraphQLのスキーマの関係性や予想される問題点について解説します。

@<chapref>{linter}ではGraphQLスキーマに対するlintツールの紹介とカスタムルールの作り方と、利用例を解説します。

@<chapref>{tips}では実際にGraphQLスキーマを設計する際に配慮すべき詳細やTipsなどを解説します。

@<chapref>{tbf-ticket}はオマケです。技術書典公式Webでの機能開発の日々を日記でお送りします。

== 予備知識1：GraphQLの概要

本書ではGraphQLの仕様については詳しく解説を行いません。
しかし、ある一定の共通理解はほしいところですので、ここに簡単に解説します。

GraphQLはGraph構造のデータから任意のデータを取り出すためのQuery Languageです。
GraphQLスキーマはデータの型とデータ間の繋がりによって定義されます。
もちろん、静的型付けなので実行前にクエリがスキーマに対して正しいかを検証できます。
さらに、クエリからどういう構造+型のデータが得られるか分かるので、フロントエンド側でデータを正しく扱っているかを静的に検査できます。
最高ですね！

SQLとGraphQLはDBへのクエリとサーバAPIへのクエリという点でだいぶ隔たりがありますが、比較してみます（@<img>{sql-graphql}）。

//image[sql-graphql][SQLとGraphQLで得られるデータ構造の比較]{
//}

SQLは@<kw>{RDB,Relational Database}に対してデータを要求する時に使います。
結果は基本的に表形式、つまり2次元のデータとして得られます。

一方GraphQLはサーバAPIに対してデータを要求する時に使います。
結果はツリー構造で得られます。

得られる形式に注目した場合、データを使った計算処理やバッチ処理を行うには2次元のデータのほうが都合がよく、画面で利用するにはツリー構造のほうが都合がよいでしょう。

なぜそういう違いになるかというと、データの結合に対する考え方が異なるのです。
RDBではテーブルに外部キーとなるカラムを持ち、クエリでもって何をどう結合するかを決定します。

対してGraphQLではスキーマの段階でどの型（RDBでいうテーブル）がどの型に広がっていくか（関連しているか）を定義します。
GraphQLでのクエリはそのスキーマに対して"こういうレイアウトでデータをおくれ"と頼むだけで、結合の仕方を変更する能力はありません。

スキーマで行える関係性の定義は柔軟です。
たとえば@<code>{User}型に@<code>{Friends(first: Int, after: String): [User!]}というフィールドがあるとします。
すると、クエリでは"User"の"Friends"の"Friends"をくれ！という具合に定義された関係性を好きな深さまで辿っていくことができます@<fn>{not-infinit}。

//footnote[not-infinit][もちろん現実的にはDBの負荷や計算量やレスポンスにかかる時間を考えどこかで制限をかける必要があります。RDBと同じ。]

クライアントからクエリを投げられるということは、SQLのように発行者が常に信用できるとは限らないため、セキュリティについて考える必要があります。
事前に危険なクエリではないかチェック可能にするため、スキーマ設計の段階で気をつけるべきポイントがあります。

== 予備知識2：どうやってGraphQLサーバは動作するのか

GraphQLサーバはresolverによってクエリをレスポンスに変換します。
どんな形のクエリでも一発で目的のレスポンスに解決（resolve）できるresolverを想像すると、とんでもなく複雑です。
普通に考えて、どういう設計にすればいいのかすら検討がつかないでしょう。

しかし、@<code>{UserID}を@<code>{User}に変換するresolverであれば実装は容易です。
@<code>{User}から@<code>{FriendIDs}を取得することはできそうですし、そこから先のresolverを利用し@<code>{User}のリストにすることもできそうです。
このように、スキーマのパーツからパーツへの変換を行う無数のresolverを作成し、それを合体させたものが、当初欲しかったクエリのresolverとなるのです。
あとは、実際のクエリにあわせて必要なresolver@<strong>{のみ}をチョイスし動作させて結果を得ます。

ものすごく無駄かつ豪華な実装では、GraphQLスキーマのすべてのフィールドに対して1つの1つresolverを作ることも可能です。
@<code>{User.id}、@<code>{User.name}、@<code>{User.email}、個別に丹念にresolverを実装するとしたら…？
1つのテーブルから一発で取得できるようなフィールドに、個別にresolverを実装したらストレスで胃に穴が3個は開くでしょう。

なので、1つの型に対して最低1つのresolverを作るのが一般的です。
もちろん@<code>{User.cpuHeavyField}のような、本来DB上に存在しないものには個別にresolverの実装を与える必要があります。
よくあるパターンとしては@<code>{User.GroupID}のような別データのIDをもつ場合、@<code>{User.Group}を作り出すために@<code>{GroupID}から@<code>{Group}に変換するresolverが必要になります。

GraphQLのスキーマとDB上のデータをつなぎ合わせるためには、resolverをどう実装するかが重要です。

//comment{
 * https://tech.mercari.com/entry/2018/10/24/111227 あたり読み返す？
 * https://github.com/vvakame/graphql-with-go-book に何書いたかも忘れた
 * https://altair.sirmuel.design/
 * https://github.com/Shopify/graphql-design-tutorial/
 ** "" と null が区別される とかはそれなーー って感じするな
 * GraphQLの操作のコピペビリティの話
 ** MutationだろうとQueryだろうとコピペして誰かに伝えることができる
 ** REST APIだとエンドポイントとデータ・フォーマットとetc... を伝える必要があり、改変も難しい
 ** Issueにログ取るもよし、Slackに貼ってChatOps的ご利益を得るもよし…
 * postmanの話？
 ** https://twitter.com/vvakame/status/1153654008952659969
//}
