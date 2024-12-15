---
layout: default
title: Lua API
nav_order: 5
has_children: true
has_toc: false
permalink: lua-api
---

# Lua API

## Table of Contents

{% comment %}
<ul>
{% for row in site.data.doc %}
<li>
{{ row["name"] }} <br>
<p>{{ row["defines"][0]["desc"] }}</p>
</li>
{% endfor %}
</ul>
{% endcomment %}

<hr>

{% for row in site.data.doc %}

<h2>{{row["name"]}}</h2>

{% if row["defines"] %}
{% assign defines = row["defines"][0] %}

{% comment %}
  defines.extends.rawdesc seems to have the description without inlined doc tags.
{% endcomment %}
{% if defines["extends"] %}
{{ defines["extends"]["rawdesc"] }}
{% else %}
{{ defines["desc"] }}
{% endif %}

{% if defines["extends"] %}
{% assign extends = defines["extends"] %}

<h3>Params</h3>

{% if extends["args"] %}

<ul>
{% for arg in extends["args"] %}
  <li>
    {{ arg["name"] }} <b>{{ arg["view"] }}</b> {% if arg["rawdesc"] %}: {{arg["rawdesc"]}} {% endif %}
  </li>
{% endfor %}
</ul>

{% endif %}

<h3>Returns</h3>

{% if extends["returns"] %}

<dl>
{% for return in extends["returns"] %}
  <dt>{{return["name"]}} <b>{{ return["view"] }}</b></dt>
  <dd>{{return["rawdesc"]}}</dd>
{% endfor %}
</dl>
{% endif %}

{% endif %}
{% endif %}

{% endfor %}