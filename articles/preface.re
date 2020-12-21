= 前書き

本書は筆者（@vvakame）がGraphQL@<fn>{graphql-url}を使った開発の中で得た知見を振り返り、解説するものです。

技術書典7で初版を出して、今回は1年ぶりに内容を更新した改訂第二版となります。
この1年で技術書典WebのGraphQLスキーマを大幅に改訂、修正しました。
なぜ修正しようと思ったのか、それに至る背景を文章に反映し、最新の知見をお届けしたいと思います。
かいつまんで変更差分が知りたい場合、GitHubで差分@<fn>{diff-v1-v2}を見ていただくのが役に立つかもしれません。

#@# TODO あとで v2.0.0 タグ打って更新
//footnote[diff-v1-v2][@<href>{https://github.com/vvakame/graphql-schema-guide/compare/v1.0.0...master}]

筆者のGraphQL歴を紹介します。
Go言語+gqlgen@<fn>{gqlgen-web}という構成で、MTC2018カンファレンスLP@<fn>{mtc2018}、技術書典Web@<fn>{tbf-web}、社内ツールのgchammer@<fn>{internal-tool}などを作成してきました。
それらで得た知見をこの本で解説します。

//footnote[graphql-url][@<href>{https://graphql.org/}]
#@# prh:disable:web
//footnote[gqlgen-web][@<href>{https://gqlgen.com/}]
//footnote[mtc2018][@<href>{https://tech.mercari.com/entry/2018/10/24/111227}]
#@# prh:disable:web
//footnote[tbf-web][@<href>{https://techbookfest.org/}]
//footnote[internal-tool][@<href>{https://engineering.mercari.com/blog/entry/20201202-7ab3acd789/}]

主にスキーマを中心に据えたベストプラクティスを紹介します。
Go言語に限らず、そしてスキーマファースト・コードファーストに限らず、読んで得るものがあるでしょう。
#@# OK gfx: 「スキーマファースト」よりも「スキーマドリブン（スキーマ駆動）」のほうが大抵の場合は実態に即している気がして好き。ただこれだと「コードファースト」に対応する一般的な語がないから「スキーマ駆動をするかどうかに限らず」みたいになるけど。まあ、好みです！（が、個人的にはこだわりがあるのでどうしても言いたかった）
#@# OK vv: GraphQLミートアップ行った時とかの口頭語も○○ファーストだったのでこのままにします！（主張は理解できる）

本書の前身として、"GraphQLサーバをGo言語で作る"@<fn>{graphql-with-go-book}と"Apolloドハマリ事件簿"@<fn>{apollo-swamped-book}があります。
若干古くなっていますが、booth.pmで販売またはGitHubで全文を公開しています。
こちらも気が向いたらぜひ読んでみてください。
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

@<chapref>{what-is-graphql}ではGraphQLが一体何者であるか、その概要をざっくり解説します。

@<chapref>{github}ではGraphQLの先駆者かつ圧倒的ユーザ支持のあるGitHubを例に、どのような設計上の工夫が行われているかを読み解きます。

#@# prh:disable:従って
@<chapref>{relay}ではGitHubも従っているRelayの仕様について解説します。Replayの仕様を学ぶことで、クライアントの都合のいいスキーマ設計のポイントを学ぶことができます。

@<chapref>{database}ではデータベースのスキーマとGraphQLのスキーマの関係性や予想される問題点について解説します。

@<chapref>{linter}ではGraphQLスキーマに対するlintツールの紹介とカスタムルールの作り方と、利用例を解説します。

@<chapref>{tips}では実際にGraphQLスキーマを設計する際に配慮すべき詳細やTipsなどを解説します。

@<chapref>{tbf-ticket}はオマケです。技術書典公式Webでの機能開発の日々を日記でお送りします。

== 謝辞（第一版）

最後まで書き終わってから、この本って巻末は日記なのであとがきがないな…？と気が付きました。
なので、ここに謝辞を書きます！バーン！

イベント一週間前の日曜日に60Pくらいの書物のレビューを依頼し、快諾してくださった皆様本当にありがとうございます…！
@wawoon_jp さん、@sonatard さん、@__gfx__ さん、@zoncoen さん、本当にありがとうございました！

== 謝辞（第二版）

#@# TODO 後で書く
TBD

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
