= 更新箇所

TODO

Organizationの導入とか色々あるけどスキーマ設計以外の話題は省く。

 * Relay Connectionを使ってなかった箇所を使うようにした
 ** 具体的に ProductInfo.images など APIで長さに制限をかけている箇所
 ** complexityの事前計算ができなくなるのでArrayにしていいのはScalarだけというルールは例外なしに守るべき
 * リファクタリングのやり方
 ** データ構造を拡張すると、以前とは違うデータの結びつきになるのは仕方がない
 ** @<code>{deprecated}@<fn>{deprecated-directive}をちゃんと使う
 *** 代替のAPIが何かをしっかり書くのがよい
 ** @<code>{eslint-plugin-graphql}@<fn>{npm-eslint-plugin-graphql}を併用しクライアント側での利用をやめる
 ** 悠長にやっていい場合
 *** @<code>{items}の返り値の型を変えたい
 *** @<code>{itemsV2}を用意し、@<code>{items}は@<code>{@deprecated(reason: "use itemsV2 insead")}とする
 *** @<code>{items}の返り値の型を変更し@<code>{itemsV2}をdeprecated、削除のステップを経て終わり
 *** APIとクライアントのリリースをあわせられる、外部から利用されていない などの条件が揃うのであれば一気にバンしたほうが楽
 ** input剥がすのが大変
 *** inputとフラット版両方をサポートするようにAPIを修正した
 *** クライアント側で修正箇所を見つけるのが案外大変だった…！
 **** deprecatedは@<code>{FIELD_DEFINITION}と@<code>{ENUM_VALUE}でのみ使える
 **** linterで見つけられない！
 *** めんどくさくなってサーバ側のinputを全部消してコード生成時のエラーを見てもぐらたたきした
 *** API側にinput使うとエラーを返す仕組みを作ったりもしたんだけどね
 **** 慎重にやるなら同じ方法でやっていけるはず
 ** enum移行が大変
 *** これも途中でめんどくさくなってドンした
 *** 最初からやっておけって感じだ
 * mutationの返り値の型をリッチにする
 ** すでに述べてた気がするけどまじでリッチにしておけ
 ** 何も返さないのは完全にギルティ 更新したデータは絶対返せるはず
 ** @<code>{NoopPayload}はクソ アンチパターン
 * なんらかのcontextに基づいて結果が変わるような処理をしないほうがよい
 ** リソースオーナーが叩いたときだけリソースを返してそれ以外はエラー みたいなやつ
 ** @<code>{@hasRole(requires: RESOURCE_OWNER)} 今結構こんなんなってる
 ** 問題点：introspection queryの結果を見るとdirectiveは見えないので情報量がない
 ** クライアント側の意思がない
 ** 今@<code>{secret: OrganizationSecret @hasRole(requires: RESOURCE_OWNER)}と@<code>{staffSecret: OrganizationSecret @hasRole(requires: STAFF)}が生えた
 ** まだマシかもしれないが、まだ不十分。staff prefixがないフィールドがどういう状態かわからないため
 ** @<code>{secret(perspective: STAFF): OrganizationSecret}とするのが正しいのではないか？というのが今の考え（まだ試してない）
 ** mutationのときどうすればいいかまだ考えてない
 * スキーマへのコメントちゃんと書いたほうが楽
 * REST APIの実装を削りまくってる話
 * Queryでinputを使わない
 ** 既存の章でinputを使う！という話を書いたけど最終的にこれはよくないことがわかった
 ** ここの意見を変えたよ、というのを書くために改訂第二版を書こうと思ったまである
 ** inputで1つの構造に引数をまとめても、結局variablesのレベルで1つの構造にまとめさせられるので無駄
 ** inputを使うと引数に対してデフォルト値が指定できなくなるのでだめ
 ** フロントエンド側のキャッシュ構造を考えたとき、inputがあると正規化しにくいのでだめ
 ** inputの定義もいっぱい消えた @<code>{clientMutationId}を持たないヤツは全部粛清
 * @<code>{nodes}と@<code>{edges}
 ** @<code>{edges}を使ったほうがキャッシュ効率よく処理できる可能性がある
 ** 前提条件としてすべてのedgesがcursorを持つ（@<code>{String!}）必要がある（そうでなければ嬉しくない）
 * enumをちゃんと使おう
 ** めんどくさいからといって@<code>{String}をよく使っていた…
 ** よくなかった
 ** Goで簡単な自動生成ツール作った
 ** フロントエンドでもタイプセーフ、API側でもタイプセーフ、やってよかった
 ** 後から導入も結構簡単だったわ
 * DBで頑張って集計していた項目をresolverで解決する
 ** あるオブジェクトの一部のフィールドを完全に計算で求めるようにする
 ** 前は重たい処理だったので裏で差分計算とかをがんばって回してた
 ** しかし、基本的にはリソースオーナーにしか見えないやつなので頑張ってメンテする価値は薄い

//footnote[deprecated-directive][@<href>{https://spec.graphql.org/June2018/#sec--deprecated}]
//footnote[npm-eslint-plugin-graphql][@<href>{https://www.npmjs.com/package/eslint-plugin-graphql}]

//comment{
 * @homebound/graphql-id-linter
 * https://joebirch.co/android/server-driven-ui-part-1-the-concept/
 ** Server Driven UI気になってる
 * GraphQL、[at]includeとか使い始めたらそれはスキーマの設計がおかしいというサイン…という仮説を思いついた($isSignedIn とかを使ってた箇所があって、[at]includeで制御してたんだけどviewerの下に移動したら制御消せることに気がついたので
 * Apollo Client v3のpaginationの話と
//}
