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

== GraphQL概要

GraphQLはGraph構造のデータから任意のデータを取り出すためのQuery Languageです。
GraphQLスキーマはデータの構造+データ間の繋がり+型によって定義されます。
もちろん、静的型付けなので実行前にクエリが正しいか、フロントエンドのコンポーネント内でデータの形式が噛み合っているかをチェックさせることができます。
最高ですね！

TODO GraphQLの概念図を書く

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
