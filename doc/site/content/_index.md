+++
title = 'Design large scale RTS games'
date = 2025-05-08T00:04:56-07:00
draft = false
+++

<div style="text-align: center;">
<h2 data-notoc="">Recoil is a battle tested open-source RTS engine that, allied with a flexible Lua API, allows you to implement the perfect UI and mechanics for your game with the ability to support thousands of complex units simultaneously.</h2>
<h4 data-notoc="">Some of the games Recoil powers:</h4>
</div>

{{< cards cols=2 >}}
{{< card title="Beyond All Reason" image="showcase/beyond_all_reason.avif" link="https://beyondallreason.info" >}}
{{< card title="Zero-K" image="showcase/zk.jpg" link="https://zero-k.info" >}}
{{< /cards >}}
{{< cards cols=2 >}}
{{< card title="Metal Factions" image="/showcase/metal_factions.jpg" link="https://metalfactions.pt" >}}
{{< card title="Total Atomic Power" image="showcase/tap.jpg" link="http://fluidplay.co/index.php/games/tap" >}}
{{< /cards >}}
{{< cards >}}
{{< card title="Tech Annihilation" image="/showcase/ta.jpeg" link="https://github.com/techannihilation/TA" >}}
{{< card title="SplinterFaction" image="showcase/splinter_faction.jpg" link="https://splinterfaction.info" >}}
{{< card title="Mechcommander: Legacy" image="/showcase/mcl.jpg" link="https://github.com/SpringMCLegacy/SpringMCLegacy/wiki" >}}
{{< /cards >}}

---

> [!NOTE]
> Recoil is a recent hard fork of [Spring](https://github.com/spring/spring) from the [105 tree](https://github.com/spring/spring/releases/tag/105.0.1), many references to it might and will be present. Overall most documented Spring API and tutorials are compatible with Recoil since they are based on the 105 tree.

## Getting started

> [!WARNING]
> This site is an early work-in-progress so content will mostly be references to Spring documentation until its own guides are written.

Want to know if recoil is right for your project? [Find out here](articles/choose-recoil/)!

The best place to get started is our [getting started](docs/guides/getting-started/) section. There are other sources, though:

- [Spring Wiki] (For information not yet ported to the docs)
- [Recoil Github Wiki]
- [Discord server]

### Download

{{< latest_release >}}

## Contributing

[![beyond-all-reason/RecoilEngine - GitHub](https://gh-card.dev/repos/beyond-all-reason/RecoilEngine.svg)](https://github.com/beyond-all-reason/RecoilEngine)

See [Development](development) for guides on how to build and
develop Recoil.

When contributing to this repository, please first discuss the change you wish
to make via [GitHub issues], our [Discord server] or any other method with the
owners of this repository before making a change.

### Thank you to the contributors of Recoil!

{{< contributors >}}

[Recoil Repo]: {{% param "repo" %}}
[GitHub issues]: {{% param "repo" %}}issues
[Discord server]: https://discord.gg/GUpRg6Wz3e
[Spring Wiki]: https://springrts.com/wiki/Main_Page
[Recoil Github Wiki]: {{% param "repo" %}}wiki
