site_name: Docs
theme:
  name: material
  font:
    text: Roboto
    code: Roboto Mono
  search: true
  features:
    - quick-links
    - navigation
    - search
    - toc
    - fullscreen
    - bookmarks
    - tabs
    - code
    - edit-on-github
    - language-selector
    - code
    - cover
    - footer
    - social-share
    - theme-switcher
nav:
  - Home: index.md
  {% if types %}
  - Types:
    {% for item in types -%}
    - {{ item.name }} : {{ item.file }}
    {% endfor %}
  {% endif %}
  {% if modules %}
  - Modules:
    {% for item in modules -%}
    - {{ item.name }} : {{ item.file }}
    {% endfor %}
  {% endif %}
  {% if globals %}
  - Globals:
    {% for item in globals -%}
    - {{ item.name }} : {{ item.file }}
    {% endfor %}
  {% endif %}
extra:
  footer: ""
