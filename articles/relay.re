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

 * Global Object Identification（オブジェクトを一意に特定可能に）@<fn>{object-identification}
 ** 全データを@<code>{ID}の値のみで特定・再取得可能に設計する
 ** @<code>{interface Node { id: ID! \}}を定義する
 ** @<code>{type Query { node(id: ID!): Node \}}を定義する
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

//footnote[object-identification][@<href>{https://relay.dev/graphql/objectidentification.htm}]
//footnote[cursor-connections][@<href>{https://relay.dev/graphql/connections.htm}]
//footnote[input-object-mutations][@<href>{https://relay.dev/graphql/mutations.htm}]
//footnote[client-mutation-id-is-dead][@<href>{https://github.com/facebook/relay/pull/2349}]

== Global Object Identification

TODO
実際にいるのか？というとちょっと微妙。
テストや開発時に便利だし、あって損はしないという印象。
グローバルにオブジェクトを一意に識別可能なIDは最初から設計しないと難しい…！
nodesの話

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
