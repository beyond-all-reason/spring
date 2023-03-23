---
layout: default
title: Synced Commands
parent: Guides
permalink: /guides/synced-commands/
---

# Synced Commands

Game developers and players can issue synced commands from chat, e.g.
("/give unit"), or `Spring.SendCommands`:

Here we provide a list of them, some commands require cheats enabled ("/cheat"):

<table>
  <tr>
    <th>Name</th>
    <th>Description</th>
    <th>Arguments</th>
  </tr>
  {% for row in site.data.synced_commands %}
    {% assign row_data = row[1] %}
    <tr>
      <td id="{{ row[0] }}">
        <a href="#{{ row[0] }}">
          {{ row[0] }}
          {% if row_data["cheatRequired"] %} <p class="label label-yellow">Cheat</p> {% endif %}
        </a>
      </td>
      <td>
        {{ row_data["description"] | xml_escape | textilize }}
      </td>
      <td>
        <dl class="dl-auto">
        {% for arg in args %}
          <dt>
            <code>{{ arg[0] | xml_escape | textilize }}</code>
          </dt>
          <dd>
            {{ arg[1] | xml_escape | textilize }}
          </dd>
        {% endfor %}
        </dl>
      </td>
    </tr>
  {% endfor %}
</table>
