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

## Generating Lua API

Have [emmylua_doc_cli](https://github.com/CppCXY/emmylua-analyzer-rust/tree/main/crates/emmylua_doc_cli) and [lua-doc-extractor](https://github.com/rhys-vdw/lua-doc-extractor) installed and available in `$PATH`.

```bash
rm -rf rts/Lua/library/generated &&
lua-doc-extractor rts/Lua/*.cpp --dest rts/Lua/library/generated &&
emmylua_doc_cli \
  -i rts/Lua/library/ \
  -o doc/site/lua-api \
  --override-template doc/emmlua-doc-cli-template/
```

See [Documenting Lua development guide](development/documenting-lua.markdown) for more info.