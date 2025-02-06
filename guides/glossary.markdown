---
layout: default
title: Glossary
parent: Guides
permalink: /guides/glossary/
---

{% assign ordered_items = site.data.glossary | sort: 'term' %}

<dl>
{% for item in ordered_items %}
  <dt><a href="{{item.url | liquify}}" target="_blank">{{item.term}}</a></dt>
  <dd>{{item.definition}}</dd>
{% endfor %}
</dl>
