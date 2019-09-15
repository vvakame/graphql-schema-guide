= データベースとの親和性

GraphQLのバックエンドはDBです。
この章では、DBとどうやって付き合っていくのか？を考えていきます。

筆者はGoogle Cloud Datastoreという分散KVS的なものを10年近く使い続けています。
なので、MySQLやPostgrasSQLに代表される@<kw>{RDB,Relational Database}の常識はもうだいぶ忘れてしまっています。
昔はSIerに勤めていたのでSQLServerとかOracleとかを扱っていた気がしなくもないんですけども。
なので、RDBに関する記述については眉に唾を付けて読んでいただけると幸いです。

== DBのスキーマとGraphQLのスキーマの相似について

DBのスキーマとGraphQLのスキーマの構造をどのように対応させるかを考えます。
筆者は基本的にはDBのスキーマとGraphQLのスキーマはほぼ一致させてしまっていいと考えています。

GraphQLのスキーマでは色々な型があり、それぞれにフィールドがあります。
フィールドは大きく3種類に分けることができます。
"名前"や"概要"といったデータ自体に何らかの価値があるもの。
別の型やリストへの参照。
未読数"99+"のような別のデータから導出できるクライアント側用の値です。
#@# wawoon: この3つの項目は箇条書きにしてあげたほうが視覚的にわかりやすいかもです

1つ目の"名前"や"概要"といった値は@<code>{String}などのスカラ型の値で表現されます。
スカラ型の値はDBに保存されているはずなので、そのまま直接翻訳することができます。

2つ目は要するに、他のエンティティとの関係性を定義するものです。
たとえばDBのスキーマ上では、別の要素のIDや、RDBであればデータ同士のひも付きを表現する中間テーブルなどです。
これらはGraphQLのスキーマにフィールドやConnectionという形で表現されるでしょう。
スカラ型の値では関係性を表現することはできませんからね。

GraphQLのクエリへのレスポンスは必ずツリー構造になります。
RDBの正規化されたデータは、そのままツリー構造と見做すことができますね。
あとはこれをGraphQLスキーマに翻訳するだけです。

このときにGraphQLスキーマ上で別データへの参照を追加することを戸惑う必要はありません。
それはGraphQLクエリが柔軟な表現力を"持たない"のでデータ参照の無限ループが生まれることはない@<fn>{graphq-spec-recursive}からです。
また、複雑なクエリに対する制約はcomplexityなどの形で制御するため、スキーマの構造によってこれを制限する必要はないからです。

//footnote[graphq-spec-recursive][GraphQLの仕様としてFragmentの展開で循環参照を作ることはできない Fragment spreads must not form cycles]

3つ目は、ある意味画期的な点です。
DB上に存在しないデータでもGraphQLスキーマ上に追加しresolverを実装してやれば、項目が存在しているかのように見えます。
クライアント側のための計算をサーバ側で肩代わりすることができるのです。
サーバ側で計算すると、クライアント側のロジックを減らし、ユーザ間でのキャッシュの共有ができることもあります。
REST APIでは計算して導出できるフィールドをレスポンスに追加することはほぼないでしょう。
それはそのフィールドを使うかわからないし、その計算コストが時にデカかったりするからです。
GraphQLの場合、クエリで要求されない限り該当のフィールドを計算する必要はありません。

クライント側から見て便利で、サーバ実装のメンテナンスのコストが問題にならないのであれば、GraphQLのスキーマはどこまでも柔軟に拡張してしまってよいでしょう。
もちろん、クライアント側から一回も使わないような構造は無駄ですし、テストの手間もかかります。
便利さとコストのトレードオフを元に、着地点を考えてみましょう。

== N+1問題への対応

N+1問題@<fn>{n+1-not-in-wikipedia}はDB次第、特にRDBでは意識しておかないとトラブルとして顕在化しやすいです。
簡単にいうと、あるデータの一覧を表示しようとした時、それにぶら下がる別のデータを取得しようとすると1+N回のDBアクセスが発生しうる、というものです。
#@# wawwon: GraphQLでは事前にどんなクエリが来るのかわからないので、多段にinner joinしたeager loadingで事前にレコード取得ができないというのがわかるといいと思います。

//footnote[n+1-not-in-wikipedia][N+1問題で調べてみてもWikipediaのページが存在していなくて驚きます。URLが出せないのでみんな自分で調べてみてね！]

技術書典でたとえてみます。
サークル一覧のデータをN件取得し、各サークルの頒布物を取得したいとします。
ベタにコードを書くと、サークル一覧の取得で1回、頒布物の取得でN回のDBアクセスが発生すると考えられます。

GraphQLではこのN+1問題が容易に発生します。
データ間の関係性をたどり、ツリー構造を要求するわけですから、@<code>{1+N}にとどまらず、@<code>{1 + N + N×M + N×M×L}くらいの闇が出現する可能性が多いにあります。
爆発ですね。
セキュリティ対策としてのcomplexityの話は@<chapref>{relay}で言及しています。

特にRDBではこの種類のデータアクセス方法はサーバに負荷がかかりやすく、なんとか1+1回のアクセスに変換する方法を考える必要があります。
頑張りましょう。

Go言語の場合、gqlgenの作者の手によるdataloaden@<fn>{dataloaden}などが利用できます。
アイディアとしては、一定時間以内に発生した同種のリクエストをプールして一箇所に集約し、IN句などを使った効率的なクエリに変換します。

//footnote[dataloaden][@<href>{https://github.com/vektah/dataloaden}]

筆者はGoogle Cloud Datastore用のラッパライブラリを作っていて、それに処理のバッチ化の機能@<fn>{mercari-datastore-batch}を持たせています。
この話は"GraphQLサーバをGo言語で作る"の本@<fn>{booth-graphql-with-go-book}で触れました。

//footnote[mercari-datastore-batch][@<href>{https://godoc.org/go.mercari.io/datastore/boom#Batch}]
//footnote[booth-graphql-with-go-book][@<href>{https://vvakame.booth.pm/items/1055228}]

他にもGoogleによるbundler package@<fn>{google-bundler}による対応もできそうです。
誰か挑戦してみてください。

//footnote[google-bundler][@<href>{https://godoc.org/google.golang.org/api/support/bundler}]

トラブルが発生してから対応を後入れするのはものすごく面倒です。
サーバを実装しはじめる段階でN+1問題への対応方法を考えて、導入しておきましょう。
ドメイン層にあまり影響を及ぼさずに利用できる仕組みにできるとなおよいです。

可能な限り、Memcachedなどへアクセスを逃がす方法も考えてみるとよいでしょう。

== ページネーションの実装について

ページネーションの実装方法について考えます。

@<chapref>{relay}で紹介したようなページネーションを実現するためには、次の情報が必要です。

 * 何件のデータが欲しいか
 * フィルタ（WHERE句）の条件は何か
 * データの参照方向は順方向か、逆方向か
 * どのデータからの続きがほしいか

このうち、何件のデータが欲しいか、フィルタの条件は何か、データの参照方向はフィールドの引数として毎回渡され、決定可能でしょう。
なので、cursorにエンコードして含めるべきは、続きのデータがどこからか？という情報です。
RDBの場合、offsetを利用するのも最悪間違いではありません。
しかし、大きなデータセットで速度面の問題があったり、途中にデータが挿入された場合に取りこぼしなどの不都合が生じます。

…という知識はあるんですがRDBを使ってないので実運用上マジで困るのかはいまいち謎です@<fn>{offset-in-datastore}。
ここでは困るという前提で話を進めます。

//footnote[offset-in-datastore][Datastoreの場合offsetベースの実装とか自殺行為なのでcursorベースの実装を強要されるのだ！]

"次の"データを取得するためのcursorには、最後に得られたデータのIDを埋め込むのがよいでしょう。
そこからそのID以降のデータの続きを取得する計算を行います。
SQLの場合、検索条件が複雑であればそのようなクエリを書くのはちょっと大変かもしれません。
"SQL cursor pagination how to"とかで調べてみるとよいでしょう。

Google Cloud Datastoreの場合、普通にcursorが取得できるのでそれを使うとよいです。

RDBに馴染んだ人がよく苦しむのが、総ページ数の表示とページジャンプの実装どうするの？という問題です。
実装を諦めるとすべてが解決します。
UIの設計を工夫して、自動読み込みによる無限スクロールの形式にするとそもそも"ページ番号"の概念が発生しません。
データ量が大量になってしまいがちな現代ではページ番号よりも検索でカバーしたほうが無難な設計かもしれません。

== 分散DBはいいぞ…！

よいと思います。
GraphQLではN+1問題や高いクエリの自由度があるため、"賢い"DBアクセスを行うのはREST APIなどと比較して難しいです。
なぜ賢くある必要があるのか？
それは、DBに余計な負荷を与えないためでしょう。
#@# wawwon: 分散DB（というかKVS）がGraphQLの実装では相性がいいことをもう少し書いてもいいかなと思いました。
#@# wawwon: 親のノードは大抵の場合子ノードのIDを知っているので高機能なクエリ言語は不要で、スケーラビリティの高いDBのほうが良いなど。

ということは、DBに負荷をかけても問題ないのであれば大雑把なアクセス方法でもOK！
DBに負荷がかかったら自動的にスケールアウトできるのであれば、困ったらスケールアウトすればいいのです。

でもお高いんでしょう？と思われる皆様のために、技術書典Webの2019年4月（技術書典6開催月）のDatastore関連費用を紹介します（@<table>{datastore-cost}）。
Memcacheにトラフィックを逃しているので実際のアクセス数はもっと多いはずですが、概ねこの程度で運用できています。
どっちかというとインスタンス代とかトラフィック代のほうが圧倒的に高いです。

//table[datastore-cost][技術書典WebのDatastore関連費用]{
SKU	使用	費用
-------------------------------------------
Cloud Datastore Read Ops Japan	53,472,602 count	¥2,181
Cloud Datastore Entity Writes Japan	570,662 count	¥44
Cloud Datastore Storage Japan	2.02 gibibyte month	¥13
Cloud Datastore Entity Deletes Japan	49,161 count	¥0
Cloud Datastore Small Ops Japan	104,655,820 count	¥0
//}

Datastoreにおける節約術はtimakinさんが書いた記事@<fn>{timakin-datastore}が詳しいです。
やっていきましょう。

//footnote[timakin-datastore][@<href>{https://medium.com/@timakin/gae-go%E3%81%AB%E3%81%8A%E3%81%91%E3%82%8B%E3%82%B3%E3%82%B9%E3%83%88%E6%9C%80%E9%81%A9%E5%8C%96-golang-c118f828b3b6}]

本当はGoogle Cloud Spannerを使いたいんですが、1ノード運用でも月間9万円とかかかりますからね…。
集計処理をリアルタイムにしたいしたいシチュエーションもちょいちょい増えているのでCloud SQLを併用しないといけない可能性を感じています。
NoSQL系は集計処理が苦手…。
