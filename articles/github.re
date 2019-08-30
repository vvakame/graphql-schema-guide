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

== URL設計とDBとunion types

TODO

== セキュリティのためのrate limitとcomplexity

TODO
relay.re でちょっと書いたのでどうするか考える
クライアントに優しくする必要があるか？
厳密にコストを計算する必要があるか？
実際に計算してみて見積もりより安かったら割引してあげるべきか？
んなこたないよ

//comment{
 * GitHub v4 APIをあがめよ
 ** GitHubがやってないことはやらない
 ** unionとかの実例
 * 命名規則の話
 ** [action][module][object] とか
 * idとかcursorとかの符号化方法
//}
