# Recoil Site

The recoil site is a jekyll application, usually hosted on github pages.

## Testing locally

Install Ruby 3.2.3 ([newer versions can cause problems](https://stackoverflow.com/a/77896791/317135)).

Inside `doc/site` run:

```bash
bundle
bundle exec jekyll build && bundle exec jekyll serve
```

Navigate to http://localhost:4000/spring

## Generating LDoc

Have [Lua Language Server](https://luals.github.io/) installed and available at your `$PATH`.

Inside `doc/site` run:

```bash
lua-language-server --configpath .luarc.doc.json --doc ../.. --doc_out_path _data
```

Keep in mind to see changes on the site you might have to restart jekyll.
