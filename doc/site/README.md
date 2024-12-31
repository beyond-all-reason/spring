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

## Generating Lua API data

### Updating the Lua definition files

First install `lua-doc-extractor`:

```bash
npm install -g rhys-vdw/lua-doc-extractor
```

At root, run:

```bash
cd rts/Lua/
rm -rf library/generated
npx lua-doc-extractor *.cpp --dest library/generated
```

### Export API data

Have [Lua Language Server](https://luals.github.io/) installed and available at your `$PATH`.

At root, run:

```bash
lua-language-server --doc . --doc_out_path doc/site/_data
```

Note: Running this using LLS 3.13.4 from a subdirectory with a relative path seems to cause duplicate class fields to be generated. Presumably a bug in LLS.

Keep in mind to see changes on the site you might have to restart jekyll.
