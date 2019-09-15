= graphql-schema-linter のルールの作り方

graphql-schema-linter@<fn>{graphql-schema-linter}はGraphQLスキーマの定義をチェックしてくれるlintツールです。
筆者の知る限り、GraphQLのスキーマに対して何らかの警告を行ってくれる著名なツールは他に存在しません。
もし知っている人がいればぜひ筆者（@vvakame）までお報せください。

このgraphql-schema-linterですが、欠点があります。
任意のスキーマへの警告をdisableにできない@<fn>{issue-disable-rules}というもので、有用な警告でも一部で無視したい場合、全体をOFFにしなければいけません。
これはかなり不便です。

//footnote[graphql-schema-linter][@<href>{https://www.npmjs.com/package/graphql-schema-linter}]
//footnote[issue-disable-rules][@<href>{https://github.com/cjoudrey/graphql-schema-linter/issues/18}]

不便なのであればパッチでも送って自分で直せよ、という話なんですが忙しさを言い訳にしてやってませんすみません…。

ともあれ、自分でルールを作れるようになると安心できる領域が広がるのでお勧めです。
この章では、graphql-schema-linterがどのようなルールをもつか、どういうアーキテクチャなのか、どのようなルールのアイディアがあるかの紹介をしていきたいと思います。

== チェック対象のスキーマをどう得るか

これまでの章ではスキーマファーストとコードファーストについてあまり言及してきませんでした。
ここで少し触れておきたいと思いますが、コードファーストでGraphQLサーバ・スキーマを構成するのは若干不利です。
組織的な側面からの良し悪しはここでは論じませんが、GraphQLの仕様的な側面からなぜ不利なのかを簡単に解説します。

コードファーストでGraphQLサーバを構成する場合、Introspection Queryを使ってエンドポイントからスキーマを引き出す形式になるでしょう。
この場合の問題点として、コメントやdirectiveに関する情報が欠落してしまいます。
もちろん、ドキュメントの一部として書いたdescriptionや、directiveの一覧は得られます。
しかし、クライアントに公開されない本当の意味でのコメントは出力できないですし、directiveがどこ付与されているかの情報も欠落します@<fn>{directive-and-introspection}。

//footnote[directive-and-introspection][@<href>{https://github.com/graphql/graphql-spec/issues/300}]

割と皆さんご存じないのですが、GraphQLでのdirectiveは外部から参照できるメタデータではありません。
よって、スキーマに@<code>{@scope(some:[READ_REPOSITORY])}のようにdirectiveを使って制御を加えた場合、これはIntrospection Queryで参照できません。
@<code>{@scope}というdirectiveが存在することは分かるけど、どこで使われているかはわからないわけです。
この仕様は本当に不便だと思いますが、とりあえずはそうなっているので仕方有りません。
外部公開用のdirectiveと内部制御用のdirectiveをキチンと表現できる仕様を頑張って導いてくれるチャンピオンの出現を祈りましょう@<fn>{graphql-spec-champion}。

//footnote[graphql-spec-champion][@<href>{https://github.com/graphql/graphql-spec/blob/master/CONTRIBUTING.md}]

ここでの問題点は、コメントやdirectiveというlinterのための指示に使えるかもしれない要素が殺されていることです。
コードファーストの場合、なんらかの工夫が必要でしょう。

== デフォルトで用意されているルール

デフォルトで利用可能なルールをざっくり説明します（@<table>{default-rules}）。
かなり有用だと思いますので、全部有効にできるように頑張るとよいでしょう。

//table[default-rules][デフォルトで用意されているルール]{
ルール名称	概要
------------------------------------------------------
@<code>{defined-types-are-used}	定義されているのに使われていない型がないか
@<code>{deprecations-have-a-reason}	@<code>{@deprecated}に理由が書いてあるか
@<code>{enum-values-all-caps}	enumの値は全部大文字のSNAKE_CASEか
@<code>{enum-values-have-descriptions}	enumの値はすべてdescriptionが書いてあるか
@<code>{enum-values-sorted-alphabetically}	enumの値の定義がソートされているか
@<code>{fields-are-camel-cased}	フィールドの名前がlowerCamelCaseか
@<code>{fields-have-descriptions}	フィールドはすべてdescriptionが書いてあるか
@<code>{input-object-values-are-camel-cased}	input objectの値の名前はlowerCamelCaseか
@<code>{input-object-values-have-descriptions}	input objectの値はすべてdescriptionが書いてあるか
@<code>{relay-connection-types-spec}	RelayのCursor Connectionsに準拠しているか
@<code>{relay-connection-arguments-spec}	同上
@<code>{relay-page-info-spec}	同上
@<code>{types-are-capitalized}	型名は先頭が大文字かどうか
@<code>{types-have-descriptions}	型はすべてdescriptionが書いてあるか
//}

== どうやって動作しているか

基本的にはルールを整えて、npmの@<code>{graphql}パッケージ@<fn>{npm-graphql}にASTやvisitorなどの処理を投げる、というシンプルな構成です。
@<href>{https://graphql.org/graphql-js/validation/#validate}を経由してルールが呼び出されるので、渡された@<code>{ValidationContext}の@<code>{reportError}を呼び出してやるだけです。
わかりやすいですね。

//footnote[npm-graphql][@<href>{https://www.npmjs.com/package/graphql}]

試しに、シンプルなルールを作成してみます。
graphql-schema-linterコマンドがデフォルトで読み込むgraphql-schema-linter.config.js（@<list>{code/linter/basic/graphql-schema-linter.config.js}）と、ルールの実装（@<list>{code/linter/basic/src/yukariKawaii.ts}）を用意します。
Visitorの利用方法はTypeScriptの型定義ファイルを見ればなんとなく分かると思うので察してください。

//list[code/linter/basic/graphql-schema-linter.config.js][graphql-schema-linter.config.jsの内訳]{
#@mapfile(../code/linter/basic/graphql-schema-linter.config.js)
"use strict";

module.exports = {
  customRulePaths: [
    `${__dirname}/src/*.js`,
  ],
  rules: [
    "yukari-kawaii",
  ],
};
#@end
//}

//list[code/linter/basic/src/yukariKawaii.ts][yukariKawaii.tsの内訳]{
#@mapfile(../code/linter/basic/src/yukariKawaii.ts)
import {
  Visitor,
  ASTKindToNode,
  ValidationContext,
  GraphQLError,
  ASTNode
} from "graphql";

export class ValidationError extends GraphQLError {
  constructor(public ruleName: string, message: string, nodes: ASTNode[]) {
    // エラーに ruleName という名前でルール名を持つ必要がある
    super(message, nodes);
  }
}

export function YukariKawaii(
  context: ValidationContext
): Visitor<ASTKindToNode> {
  return {
    FieldDefinition: v => {
      if (v.name.value !== "yukari") {
        return;
      }

      context.reportError(
        new ValidationError(
          "yukari-kawaii",
          `${v.name.value}という名前は可愛すぎます！`,
          [v]
        )
      );
    }
  };
}
#@end
//}

この例では@<code>{FieldDefinition}、つまり、フィールドの定義をチェックし、名前が@<code>{yukari}だったらエラーにします。
@<img>{yukari}を見て分かるとおりとてもかわいいです。

//image[yukari][ゆかりさんのご尊顔][scale=0.3]{
//}

試しに@<list>{code/linter/basic/test/test.graphql}をチェックすると次のような結果になります。

//list[code/linter/basic/test/test.graphql][スキーマの例]{
#@mapfile(../code/linter/basic/test/test.graphql)
type Query {
  yukari: String
}
#@end
//}

//cmd{
$ npx graphql-schema-linter ./test/*.graphql
（略）/test/test.graphql
2:3 yukariという名前は可愛すぎます！  yukari-kawaii
//}

このように、カスタムルールを定義すること自体は非常に簡単です。
まずは、graphql-schema-linterのリポジトリを見て、各ルールの実装を見てみるとよいでしょう。
さらに詳細な挙動を知りたい場合は、"node --inspect-brk"などでググってデバッガの使い方を学習すると、処理を追いやすく理解しやすいです。
ChromeのDevToolsを使うやり方が王道かつ罠が少ないのでお勧めです。

Node.jsに慣れていない人には若干酷な話ですが、GraphQL界隈のツールの標準はどうしてもNode.jsに偏っています。
なので、簡単なJavaScriptやTypeScriptの読み（できたら書き）と、デバッガの使い方は覚える価値があります。

== 広げようカスタムルールの輪

このように簡単にカスタムルールが作れますので、みんなで便利なルールを作って共用しましょう！
そのためにルールのメンテをするリポジトリがほしいなぁ、と思っているんですがまだ作っていません。
誰か作ってくれるといいと思います。

というわけで、まずは筆者が技術書典Webで運用しているカスタムルールを解説してみます。
大変申し訳ないですがカスタムルールの実装は公開していません…。
先に警告を無視するパッチを作って、ダメだったら自分でlinter書き直して、いやApolloのクライアント側スキーマを考慮するとeslintプラグイン前提でやったほうが…？とか考え始めたらめんどくさくなったからです。
罪深い…。

御託はいいから先に公開しろゴルァ！とかいわれたら公開すると思います…。
技術書典7が終わった後に…！

==== @<code>{field-name-matches-input-type}

筆者はRelay系の仕様で要求されない限り、引数を@<code>{input}という名前にしてinput objectを定義しています。
ここで、input objectの命名規則を フィールド名+"Input" または 元オブジェクト名+フィールド名+"Input" というルールにすることにしました。

たとえば、@<code>{circles}というフィールドがあり、そこに引数を定義する場合は@<code>{input: CirclesInput}のような名前にするわけです。
フィールド名が決まるとinput objectの名前が自動的に決まるので、そこそこ役に立つルールだと思います。

==== @<code>{input-type-naming}

input objectの命名規則は必ず末尾に "Input" で終わらなければいけないことを強制するルールです。
ハンガリアン記法っぽい気もしますが、今のところ悪くないです。

==== @<code>{is-input-requires}

引数を@<code>{input: CirclesInput}のような形式にする、と少し前に書きましたが、他にもNonNullである@<code>{input: CirclesInput!}も選択肢に入ります。
これを自動的に決定するため、input objectにNonNullの値が1つでもあれば、自動的にinputもNonNullに、さもなくば省略可能であることを強制します。

このルールはかなり便利で、合理的です。
inputの省略可不可と、input objectの値がNonNullかどうかは基本的には一致します。

精神がゆるふわの状態でコーディングをしていると、このあたりの配慮が意識から漏れる場合があります。
そして、クエリを書く時に@<code>{input: {\}}などと書かされストレスを溜めてしまうわけです。
このストレスを避けるためのルールです。

==== @<code>{list-must-be-relay-connection}

フィールドの値がリストの場合、これをRelayのCursor Connectionsの仕様に準拠するよう怒ってくれるルールです。
基本的に、complexityを計算可能にするためにこのルールには従うようにするべきです。

このルールのチェック除外対象となるルールが少しあります。

 * オブジェクト名の末尾が "Payload" なもの
 * フィールド名が "edges" の場合
 * フィールド名が "nodes" の場合
 * リストの要素がスカラ型の場合

これに当てはまる場合は、complexityを計算する上で障害にならないからです。

このルールを無視すると後で本当に辛い目にあります。
筆者は現在進行系で辛い目にあっています。
@<code>{CircleExhibitInfo}はDB上に@<code>{tagID: [ID!]!}的な構造を持ち、これは"ついで"に取れます。
なので安易な気持ちで@<code>{tags: [Tag!]!}という定義をルールに逆らって作ってしまいました。
すると@<code>{Tag}はいくつかのさらなる別の型への展開を持ち、ここでcomplexityの計算が崩壊しました。
教訓として、DBから1アクションで取れるリストデータであっても、非スカラ型の場合インメモリでCursor Connections相当の構造に変換するべきです。
つらいです。

==== @<code>{list-name-must-be-plural}

リストもしくはConnectionが返ってくるフィールドの場合、末尾に"List"がついたら怒ってくれるルールです。
スキーマ設計初期に@<code>{circleList}と@<code>{circles}で命名規則がブレブレだったので作りました。
GitHubと同じく、複数形の@<code>{circles}に統一していきたいところです。

==== @<code>{mutation-delete-id}

Mutationで名前に@<code>{delete}または@<code>{remove}を含む場合、返り値に@<code>{deletedXxxID}または@<code>{removedXxxID}という名前のフィールドがなければいけないというルールです。
これはRelayのMutations updaterに配慮したルールで、キャッシュから該当データを消せるだけの情報をかならず返させる意図があります。

==== @<code>{mutation-field-naming}

Mutationで名前の先頭に@<code>{insert}がついていたら弾くルールです。
@<chapref>{github}でも触れた、"DBへの操作"っぽい名前を避ける意図があります。

==== @<code>{names-must-input}

フィールドの引数でinput objectを使う場合、引数の名前を@<code>{input}縛りにするルールです。
これはスキーマ全体で一貫性を持たせるためのルールです。

==== @<code>{staff-field-naming}

これはスタッフ権限持ちユーザのためのAPIを確実に保護するためのルールです。
フィールド名の先頭が"staff"の場合、@<code>{@hasRole(requires: STAFF)}のdirectiveが指定されていないと怒るルールです。

技術書典Webでは、ドメインロジック内でも権限チェックを行っています。
が、念の為GraphQLのスキーマ上でも権限を設定し、directiveの指示に従いログインユーザの権限を都度チェックしています。
このチェックを忘れずに行うためにこのルールは必要です。
たまに指摘されてあっぶねー！ってなってます。
