---
layout: default
title: Home
nav_order: 1
description: "Recoil is an RTS engine designed for game flexibility and large scale."
permalink: /
---

# Design large scale RTS games
{: .fs-9 }

Recoil is a battle tested open-source RTS engine that, allied with a flexible
Lua API, allows you to implement the perfect UI and mechanics for your game
with the ability to support thousands of complex units simultaneously.

Some of the games powered by Recoil: [Beyond All Reason], [ZeroK], [TA Prime]
and [Metal Factions].

{: .fs-6 .fw-300 }

[Get started now](#getting-started){: .btn .btn-primary .fs-5 .mb-4 .mb-md-0 .mr-2 }
[View it on GitHub][Recoil repo]{: .btn .fs-5 .mb-4 .mb-md-0 }

---

{: .warning }
> Recoil is a recent hard fork of [Spring] from the [105 tree], many references to it might and will be present. Overall most documented Spring API and tutorials are compatible with Recoil since they are based on the [105 tree].

## Getting started

{: .note }
This site is an early work-in-progress so content will mostly be references to
Spring documentation until its own guides are written.

References:

- [Spring Wiki]
- [Recoil Lua API]
- [Recoil Github Wiki]

Get the latest release [here](https://github.com/beyond-all-reason/spring/releases/tag/spring_bar_%7BBAR105%7D105.1.1-1354-g72b2d55).

## Building the Engine

See the [Recoil Github Wiki] on how to build the engine locally or with Docker.

## Contributing

When contributing to this repository, please first discuss the change you wish
to make via [GitHub issues], our [Matrix Room] or any other method with the
owners of this repository before making a change.

### Thank you to the contributors of Recoil!

<ul class="list-style-none">
{% for contributor in site.github.contributors %}
  <li class="d-inline-block mr-1">
     <a href="{{ contributor.html_url }}"><img src="{{ contributor.avatar_url }}" width="48" height="48" alt="{{ contributor.login }}"></a>
  </li>
{% endfor %}
</ul>

[Recoil repo]: https://github.com/beyond-all-reason/spring
[GitHub issues]: https://github.com/beyond-all-reason/spring/issues
[Beyond All Reason]: https://beyondallreason.info
[ZeroK]: https://zero-k.info
[Spring]: https://github.com/spring/spring
[Metal Factions]: https://metalfactions.pt
[TA Prime]: https://www.fluidplay.co/tap.html
[105 tree]: https://github.com/spring/spring/releases/tag/105.0.1
[Matrix Room]: https://matrix.to/#/#recoil-rts:matrix.org
[Spring Wiki]: https://springrts.com/wiki/Main_Page
[Recoil Lua API]: /spring/ldoc
[Recoil Github Wiki]: https://github.com/beyond-all-reason/spring/wiki
