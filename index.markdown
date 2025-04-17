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

Some of the games powered by Recoil: [Beyond All Reason], [Zero-K],
[Total Atomic Power], [Tech Annihilation] and [Metal Factions].

{: .fs-6 .fw-300 }

[Get started now](#getting-started){: .btn .btn-primary .fs-5 .mb-4 .mb-md-0 .mr-2 }
[View it on GitHub][Recoil repo]{: .btn .fs-5 .mb-4 .mb-md-0 }

---

{: .warning }
> Recoil is a recent hard fork of [Spring] from the [105 tree], many references
to it might and will be present. Overall most documented Spring API and
tutorials are compatible with Recoil since they are based on the [105 tree].

## Getting started

{: .note }
This site is an early work-in-progress so content will mostly be references to
Spring documentation until its own guides are written.

References:

- [Spring Wiki]
- [Recoil Lua API]
- [Recoil Github Wiki]

### Download

The latest stable release is `{{site.data.latest_release.name}}` available at:

{% assign releases = site.data.latest_release.assets | where_exp: "asset", "asset.browser_download_url contains 'minimal-portable'" %}

{% for rel in releases %}
- [`{{rel.name}}`]({{rel.browser_download_url}})
{% endfor %}

See the [release page]({{site.data.latest_release.html_url}}) for more options.

## Contributing

See [Development](development.markdown) for guides on how to build and
develop Recoil.

When contributing to this repository, please first discuss the change you wish
to make via [GitHub issues], our [Discord server] or any other method with the
owners of this repository before making a change.

### Thank you to the contributors of Recoil!

<ul class="list-style-none">
{% assign contributors = site.data.non_coder_contributors | concat: site.github.contributors %}
{% assign contributors_size = contributors | size %}
{% assign shuffled_contributors = contributors | sample: contributors_size %}
{% for contributor in shuffled_contributors %}
  <li class="d-inline-block mr-1">
     <a href="{{ contributor.html_url }}"><img src="{{ contributor.avatar_url }}" width="48" height="48" alt="{{ contributor.login }}"></a>
  </li>
{% endfor %}
</ul>

[Recoil repo]: {{site.gh_edit_repository}}
[GitHub issues]: {{site.gh_edit_repository}}/issues
[Beyond All Reason]: https://beyondallreason.info
[Zero-K]: https://zero-k.info
[Spring]: https://github.com/spring/spring
[Metal Factions]: https://metalfactions.pt
[Total Atomic Power]: http://fluidplay.co/index.php/games/tap
[Tech Annihilation]: https://github.com/techannihilation/TA
[105 tree]: https://github.com/spring/spring/releases/tag/105.0.1
[Discord server]: https://discord.gg/GUpRg6Wz3e
[Spring Wiki]: https://springrts.com/wiki/Main_Page
[Recoil Lua API]: {{site.baseurl}}{% link lua-api/docs/index.md %}
[Recoil Github Wiki]: {{site.gh_edit_repository}}/issues
