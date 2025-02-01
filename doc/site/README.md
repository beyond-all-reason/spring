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

See the [Lua Language Server](doc/site/guides/lua-language-server.markdown) guide for info on manually regenerating the Lua library.

### Export API data

Have [Lua Language Server](https://luals.github.io/) installed and available at your `$PATH`.

```bash
# This step should be removed when https://github.com/LuaLS/lua-language-server/issues/2977 is resolved.
cp doc/site/.luarc.doc.json recoil-lua-library/.luarc.json
cd recoil-lua-library/
lua-language-server --doc . --doc_out_path doc/site/_data
```

Keep in mind to see changes on the site you might have to restart jekyll.
