= 前書き

前書きなのだ。
やっていきなのだ。

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
//}
