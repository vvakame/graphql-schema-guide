={github} GitHub v4 APIに倣え！

GitHubはGraphQLを使ったv4 APIを提供しています@<fn>{github-v4}。
GitHubはおそらく皆さん使っているでしょうから、GitHub自体の説明などは省いてよいでしょう。
どういうデータ構造があるのかも、ある程度想像できるのではないでしょうか？

//footnote[github-v4][@<href>{https://developer.github.com/v4/}]

GitHubのAPIはさすが数多の開発者が利用しているサービスだけあって、さまざまなところまでスキーマ設計上の配慮が行き届いています。
筆者が色々と試して経験を積んだ結果、GitHubのプラクティスを読み解き、自分のスキーマに反映させる価値は大いにあります。

この章では、GitHubがどのような工夫を行っているかを解説していきます。

== Relayへの準拠

TODO

== 命名規則について

TODO

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
