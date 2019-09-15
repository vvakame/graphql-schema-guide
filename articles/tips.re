= スキーマ設計の実践と考察

この章では、実践した上での知見や反省、他の章に分類できなかった内容、筆者が挫折していることを紹介していきます。

=={frontend-matter} 画面の都合とGraphQL

GraphQLのクエリは非常に柔軟な表現力を持ちます。
ゆえにサーバ側の特別な助けなしに画面に必要なデータを取得できます。
クライアント側とサーバ側、実装の詳細を相互に気にする必要がなくなります。

一方、Mutationはそうもいきません。
APIにどういうスキーマを提供するかで画面側の組みやすさが変わってしまう場合もあります。

たとえば、@<code>{CircleExhibitInfo}（サークル情報）と@<code>{ProductInfo}（頒布物情報）を一括で新規登録できる画面を作りたいとします。
技術書典での現状の設計では、ProductInfoはCircleExhibitInfoの所有物という扱いになっています。
ということは、ProductInfoを新規登録するにはCircleExhibitInfoのIDが必要になるわけです。

Mutationがクライアントの都合を考えずに組まれている場合、@<code>{createCircleExhibitInfo}を呼んだ後に@<code>{createProductInfo}を呼ぶ必要があります。
実行順序に依存関係が発生する上、途中でエラーが発生した場合のリトライや復帰処理も面倒です。
クライアント側で考えたり吸収すべき問題が多すぎます。

これを解決するため、@<code>{batchCreateCircleExhibitInfoAndProductInfos}のようなAPIを作ってもよいでしょう。
美的観点で見るとヤバそうですが、クライアント側からすると1回のアクセスで完結するので便利です。

GraphQLの仕様として、Mutationのある部分の返り値を動的な変数として別の場所で参照できる、という仕様@<fn>{spec-dynamic-var}を提案している人がいます。
クライアント側からすると嬉しいかもしれませんが、サーバ側のフレームワーク開発者から見ると地獄みがあります。
もし実現されたとしても、結局トランザクションで悩む気もします。

//footnote[spec-dynamic-var][@<href>{https://github.com/graphql/graphql-spec/issues/583}]

== 架空のフィールドをひねり出す

この本でもすでに何度も触れている気もしますが、重要な考え方なので再度書きます。
GraphQLスキーマはDBのスキーマにある程度一致しますが、完全に一致する必要はありません。

GraphQLとDBの間にはresolverという定義と実装の間を橋渡しする存在があります。
resolverはプログラムコードとして表現されるので、当然のことですが計算能力があります。
何らかのデータを変換したり、集計したりすることができるわけです。

たとえば、@<code>{contentMarkdown: String!}というフィールドがあって、データをMarkdownで保持しているとします。
MarkdownをHTMLに変換するライブラリを使えば、@<code>{contentMarkdown}を元データとする@<code>{contentHTML: String!}を定義・実装できます。

もちろん、単なるスカラ値だけではなく、別の型との関係性も作り出すことができます。
"こんな風にデータが取れたらもっと楽なのに"を実現する能力がGraphQLにはあります。
GraphQLという棍棒を大いに振り回していきましょう。

== スキーマ定義を複数ファイルに分割する

スキーマ定義を単一のファイルで育てていくのは限界があります。
試しにやってみると、少なくともエディタ側からかなり強力なGraphQLサポートがないと編集したい箇所を探すだけで一苦労です。

すると、スキーマを分割して複数のファイルで定義したくなります。
GraphQLはスキーマを分割して定義するための仕様を持っています。
それが@<code>{extend}です。
この仕様を活用すると、@<code>{Query}型の定義を複数箇所に分割して行えます。

あとは、複数のファイルを適当にconcatすれば1つのスキーマファイルが手に入ります。
大抵のツールは複数のファイルを内部でいい感じにマージして処理する機能がついていることでしょう。

まずは、ベースとなる定義を見てみます（@<list>{code/tips/schemaExtend/schema.graphql}）。
@<code>{extend}を使うためには、大本の定義が必ず1つ必要です。
さらに、定義には必ず子要素（フィールド）の定義が必要になります。

//list[code/tips/schemaExtend/schema.graphql][schema.graphql]{
#@mapfile(../code/tips/schemaExtend/schema.graphql)
type Query {
  node(id: ID!): Node
}

type Mutation {
  noop(input: NoopInput): NoopPayload
}

interface Node {
  id: ID!
}

input NoopInput {
  clientMutationId: String
}

type NoopPayload {
  clientMutationId: String
}
#@end
//}

Queryの定義には@<chapref>{relay}のGlobal Object Identificationに準拠するのであればフィールドが1つあります。
しかしMutationにはどんなスキーマ設計であろうが必ず存在するフィールドはありません。
筆者は@<code>{noop(input: NoopInput): NoopPayload}を定義してごまかしています。

その他、全体で共有して使うinterfaceやdirectiveなどの定義もここで行っています。

次に、個別の型をそれぞれのファイルに定義していきます（@<list>{code/tips/schemaExtend/event.graphql}）。
ファイルをどういう粒度に分割するかですが、だいたいDBのカインドやテーブル単位で1:1に対応させています。
バックエンドがマイクロサービス群の場合、もうちょっと工夫の余地がありそうです。

//list[code/tips/schemaExtend/event.graphql][event.graphql]{
#@mapfile(../code/tips/schemaExtend/event.graphql)
extend type Query {
  events(first: Int, after: String): [Event!]! # Relay仕様は紙面の都合で無視
}

extend type Mutation {
  staffCreateEvent(input: StaffCreateEventInput!): EventPayload
  # staffUpdateEvent ... などが続くはずだが省略！
}

type Event implements Node {
  id: ID!
  name: String!
}

input StaffCreateEventInput {
  clientMutationId: String
  # 紙面を使うので以下省略！
}

type EventPayload {
  clientMutationId: String
  event: Event
}
#@end
//}

さらに既存の型を拡張していきます。
DBのスキーマ上は、@<code>{CircleExhibitInfo}は@<code>{eventID}で@<code>{Event}への紐づきを保持しています。
GraphQLスキーマ上での定義を行う箇所も、これに倣います（@<list>{code/tips/schemaExtend/circleExhibitInfo.graphql}）。

//list[code/tips/schemaExtend/circleExhibitInfo.graphql][circleExhibitInfo.graphql]{
#@mapfile(../code/tips/schemaExtend/circleExhibitInfo.graphql)
type CircleExhibitInfo {
  id: ID!
  name: String
}

# DBスキーマ上、CircleExhibitInfoがeventIDを持つ
# データとしての主体はCircleExhibitInfo側なので、Eventをこのファイルで拡張する
extend type Event {
  # Relay仕様は紙面の都合で無視
  circles(first: Int, after: String): [CircleExhibitInfo!]!
}
#@end
//}

親側である@<code>{Event}のファイルに@<code>{circles}を定義してしまうと、そのうち破綻してしまいます。
@<code>{Event}は大人気の親で、さまざまなデータとの関係性定義が同一ファイルに大集合してしまうのです。
そうすると、自然と定義されるinput objectや返り値用のtypeが溢れることになります。
そして、それらの型同士はなんの関係性も持たないのです。

であれば、親への参照をもつ@<code>{CircleExhibitInfo}側に関係性が記述してあるのは自然であり、管理もより容易くなるでしょう。

このやり方でもあまり上手には解決できていない問題もあります。
@<hd>{frontend-matter}で触れたような、画面のために複数の型を同時に更新するようなMutationを作る場合、それをどこに置くべきか悩ましいのです。
今のところ、主眼となる型と同じファイルに置いていますが、ファイルの太り方に偏りが生まれてしまっています。

最後にファイル同士の関係性を定義する仕様を紹介しておきます。
Prismaのimport syntaxの提案@<fn>{prisma-import}があるのですが、あまり受け入れられておらず、流行っていません。

//footnote[prisma-import][@<href>{https://oss.prisma.io/content/graphql-import/overview}]

== エラーハンドリングのパターン

結論からいうと、今のところベストな解は得られていません。
REST API時代も入力値をエラーにする時どうするか悩んだまま解が得られていないのですが、それが続いているだけともいえます。

目指すエラーハンドリングの目標としては次のような感じです。

 * 可能な限り開発者の手を動かさせずにわかりやすいエラー表示をしたい
 ** フロントエンドの入力要素とGraphQLへのvariableを半自動的に紐付ける
 ** GraphQLから返ってきたエラーの箇所と入力要素を紐付ける
 ** 入力要素をエラー有状態に変える、エラー文言を表示する

実現されたら若干オーパーツ感ありますね。

現実的な運用として、サーバ側のドメイン層で出したエラーを@<code>{extensions}に変換して返しています。
ここの書式はREST APIで返していたものとまったく同一なので、REST API時代のエラーハンドリング機構をほぼそのまま再利用しています。

実際のエラーの例は@<list>{error-extensions}のような感じです。
画面側は@<code>{errorCode}を見て画面に何か表示させています。
どの入力が起因になったエラーか、などは現状ではフォローできていません。
クライアント側でサーバ側と似たようなエラーチェックを実装する必要が生じています。
当たり前といえば当たり前です。

//list[error-extensions][エラーの書式]{
{
  "errors": [
    {
      "message": "status 400: E4-0009 - bad request. detail: invalid id syntax",
      "path": [
        "updateCheckedCircle"
      ],
      "extensions": {
        "code": 400,
        "errorCode": "E4-0009",
        "text": "bad request",
        "type": "https://techbookfest.org/api/error-type"
      }
    }
  ],
  "data": {
    "updateCheckedCircle": null
  }
}
//}

もっと楽がしたい！！！

== エラーを返していいとき悪いとき

エラーハンドリングの話を書いたので少し触れておきます。

筆者が行ったREST APIからの移行初期のGraphQL実装は、直感的ではないエラーを返していました。
それはドメイン層の権限チェックの結果であったり、スキーマ上からは汲み取れない裏側のフラグや、ログインユーザの種別や条件の組み合わせに依存していたりしました。

GraphQLにおいては、予測可能な結果を返さないのは罪です。
スキーマから汲み取れないことは行ってはいけません。
クエリがスキーマに対して正しいのであれば、必ず結果を返すか、さもなくばわかりやすい、見ただけで問題を解決できる親切なエラーを返しましょう。
見せたくないデータがある場合、それはクエリ上で工夫せずとも自然に除外され公開されるよう設計するべきです。

技術書典のREST APIでは、@<code>{visibility=staff}とクエリパラメータに指定すると見える情報が増えたり、使える検索条件が増えたりする設計でした。
これをGraphQLスキーマの設計にそのまま持ち込むと、同一のフィールドに対して一般用の使い方とスタッフ用の使い方が混在してしまいます。
一般ユーザは何をしてはいけないのか？使ってよいパラメータはどれか？
これを理解するのは難しく、使い物になりませんでした。
最終的に一般ユーザ用とスタッフ用のフィールドを根本的に分け、それぞれ独立したresolverとすることで解決しました。

教訓として、スキーマやエラーメッセージを見てエラーの原因が理解できず、修正方法がわからず、ソースコードを見に行かねばならなかったら負けです。
クライアント側の人間は激烈に怒ってしかるべきだと思います@<fn>{recursive-anger}。

//footnote[recursive-anger][つまりクライアント担当時の僕がサーバ担当時の僕にバチクソに怒りました。うるせー自分で直せ！自分がやったんだろ！自己参照型の喧嘩は虚しいですね]

DBへのクエリの条件も組み合わせでエラーにしたい場合があるでしょう。
その場合も、見てデバッグできるようなメッセージを提示してあげたいところです。
基本的には"使い方が悪い"というエラーを見る人はフロントエンドの開発者であり、エンドユーザを考慮する必要はないでしょう。

== フィールドに他の型のIDをもつか？

たとえば、@<code>{CircleExhibitInfo}がDB上で@<code>{eventID: ID!}というフィールドをもつとします。
このとき、GraphQLスキーマ上では@<code>{event: Event!}を公開して@<code>{eventID}は隠すのか？という議論があります。

現時点では筆者は解を持っていません。
今のところ、@<code>{eventID}は隠して@<code>{event}だけ露出し、IDが必要なら@<code>{event.id}を参照すればよいでしょ？と思っています。

これには実装上のトレードオフが含まれています。
@<code>{eventID}を返す場合、@<code>{CircleExhibitInfo}がそのデータを持っているので、余計なDBアクセスは発生しません。
これを隠す場合、一般的なresolverの実装だと一度@<code>{Event}のデータをDBから取得する必要があるでしょう。
とても賢いGraphQLライブラリを考えると、無駄なDBアクセスを発生させないこともできそうですが、"id以外のフィールドにアクセスしたらDBアクセスが必要"ということを表現できるresolverはちょっと難しそうです@<fn>{let-me-know-smart-resolver}。

//footnote[let-me-know-smart-resolver][もしそういう頭のいい実装の実例を知ってたら筆者までお報せください]

今のところ、DBアクセスはだいたいmemcachedのキャッシュにhitするだろう、と期待して隠してしまっています。
ここについては将来、考え方が変わるかもしれない@<fn>{for-example-real-beaten-me}ですね。

//footnote[for-example-real-beaten-me][具体的には現実的な要求に僕が打ちのめされたとき]

== 虚空から型をひねり出さない

技術書典では意図せぬ情報の露出を防ぐため、公開用の@<code>{CircleExhibitInfo}と非公開の@<code>{CircleExhibitInfoSecret}に内部的に分かれています。
DB上分かれているデータですが、REST APIではJSONを合成する形で1つのデータ種別であるように見せかけている箇所が多々ありました。

当初、このREST APIの設計に引っ張られて2つのテーブルのデータを合成した型をGraphQL上に作ろうと考えました。
そして、案の定resolverの実装がぐちゃぐちゃになってしまい諦めることになりました。
最終的には@<code>{CircleExhibitInfo}に@<code>{secret: CircleExhibitInfoSecret}を作って解決しました。
既存のAPIとの互換性は考えず、GraphQLではGraphQLらしく設計することが大事でした。

別の事例では、DB上に存在しない値を型として切り出そうとしたことがありました。
resolverで値を計算してひねり出すのですが、@<code>{id: ID!}が存在しない場合、クライアント側でのキャッシュ操作が難しくなります。
そのため、これを避けるか仮のid値をひねりだすようにしたほうがよいかもしれません。

== ドメイン層とGraphQL resolverのレイヤを分けて考える

スキーマの話とはちょっと外れますが、スキーマに対する実装をどう用意するか？について考えてみます。

GraphQLはクライアント側に対するプレゼンテーション層であり、ビジネスロジックそのものではありません。
また、GraphQLレイヤーをいつでも剥がして別のものに載せ替えられるようにしておくのは悪いことではないでしょう。

ドメイン層のロジックと、GraphQLのレイヤーの噛み合わせは必ずしも簡単ではありません。
筆者の使っているGo言語+Cloud Datastoreの構成では、stringをDBに保存する時、NULLにするという概念が薄いです。
値が空ならば、空文字列が保存されます。
この考え方はGraphQLの世界観とは相性が悪いです。
そこで、Go言語+DBの慣習に従う世界観とGraphQL世界観に従う層をどこかで明確に線引きするのがよいです。

これを行うためには、スキーマファーストだと都合がよいです。
コードファーストだと、コードを書く楽さに精神が引っ張られてしまいがちです。
スキーマファーストであれば、APIとして妥当であることに心を砕き、実装時にはスキーマ設計者を呪う@<fn>{dont-curse-others}ことで設計をきれいに保つことができます。

//footnote[dont-curse-others][設計者が自分じゃない場合は安易に呪うのはやめましょう]

特に、Mutationの引数に何を要求するかは悩ましいところです。
GraphQLスキーマ的には更新したい箇所の値だけ渡せばよい構造にしたいです。
もし更新時にすべてを要求すると、Queryの時点で画面の描画に必要ではない値まで取得する必要が生じます。
それはGraphQLのよさを犠牲にしてしまいます。

今挑戦してみたいのがgRPCのサーバをプロセス内に立てて、GraphQLレイヤーからgRPCクライアントとしてアクセスする方法です。
大規模化していくと複数のサービスに分割する日が来るでしょうし、そのときサービス間の通信にgRPCが使えると便利です。
であれば、プロセス内でUNIXドメインソケットやメモリ内ネットワーク@<fn>{in-memory-network-on-go}でgRPCサーバ&クライアントという形であらかじめドメイン層を独立させておくのも面白そうです。

//footnote[in-memory-network-on-go][筆者が試したのは@<href>{https://github.com/akutz/memconn}とか]

== directiveの利用とドメイン層でのチェック

directiveの使い方を考えます。
@<code>{hasRole}や@<code>{scope}のようなdirectiveを定義し、ミドルウェアできっちりチェックするとします。
それでも、筆者はドメイン層でもう一度同じ権限やスコープのチェックを行っています。

これにはいくつかの（若干神経質な？）理由があります。

スキーマにdirectiveをつけて管理する場合、漏れが怖いです。
適切にdirectiveが付与されているかを評価するのは人間の主観に寄りますし、テストも難しいです。

次にdirectiveによる制御をフレームワークが行うことに全幅の信頼が置けないこと。
たとえば、筆者の使っているgqlgenというライブラリではまだまだdirectiveでカバーされていない範囲があります@<fn>{gqlgen-directive-limitation}。
主にSubscription周りなどのサポートがまだです。
筆者はSubscriptionを使っていないのですが、いつ罠にハマるかはわかりません。

//footnote[gqlgen-directive-limitation][@<href>{https://gqlgen.com/reference/directives/}]

最後にGraphQL以外のAPIを提供するとき、GraphQLと同様のチェック機構を後から組むこむのは辛いです。
ですので、最初からドメイン層に統一的に組み込んでおくとよいでしょう。
筆者は稀にこの戒めを破り、resolver内にDBへのクエリを直接書いてしまいます。
ドメイン層に該当の処理を書くのが面倒だからなのですが、本来であれば自省するべきです…。

//comment{
結局、directiveによる制御とドメイン層での制御を別途行うのであれば、directiveによる制御は不要では？という考え方もありそうです。
@<chapref>{linter}で少し触れたとおり、Introspection Queryにはどのフィールドや型にdirectiveが付与されているかを伺い知る方法はありません。
よって、ドキュメントにどういう制約がかかっているかを明文化する必要があります。
//}

//comment{
 * ロギングの話
//}
