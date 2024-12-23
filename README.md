# Recoil Site

The recoil site is a jekyll application, usually hosted on github pages.

## Testing locally

Have a reasonably recent version of Ruby. Inside `doc/site` run:

```bash
bundle
bundle exec jekyll build && bundle exec jekyll serve
```

Navigate to http://localhost:4000/spring

## Generating LDoc

Have [LDoc](https://github.com/lunarmodules/LDoc) installed or available at your `$PATH`.

Inside `doc/LDoc` run:

```bash
ldoc -c config.ld .
```

Keep in mind to see changes on the site you might have to restart jekyll.
