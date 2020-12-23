# GraphQLスキーマ設計ガイド 第二版

[技術書典7](https://techbookfest.org/event/tbf07)で作った書籍、「GraphQLスキーマ設計ガイド」の全文が置かれたリポジトリです。
[技術書典10](https://techbookfest.org/event/tbf10)で改訂第2版を出す予定です。

[技術書典 オンラインマーケット](https://techbookfest.org/product/5680507710865408)または[booth.pm](https://vvakame.booth.pm/items/1576562) で500円で販売しています。
自分でビルドするのが面倒な人はこちらからお買い求めください。

ビルドの仕方は、このリポジトリのCircle CIの設定ファイルとかを見て察してください。
だいたい `./build-in-docker.sh` を叩けばなんとかなるはずです。

```
$ REVIEW_CONFIG_FILE=config-ebook.yml npm run pdf
```

で電書用のトンボが入ってないPDFが作れます。
表紙データはリポジトリに含めていないのでそのままだとエラーになります。
`articles/config-ebook.yml` の `coverimage` の指定をコメントアウトしてください。
