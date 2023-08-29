# BlueHeronTransportHCIDev

[![Hex version](https://img.shields.io/hexpm/v/blue_heron_transport_hcidev.svg "Hex version")](https://hex.pm/packages/blue_heron_transport_hcidev)
[![API docs](https://img.shields.io/hexpm/v/blue_heron_transport_hcidev.svg?label=hexdocs "API docs")](https://hexdocs.pm/blue_heron_transport_hcidev/BlueHeronTransportHCIDev.html)
[![CircleCI](https://circleci.com/gh/blue-heron/blue_heron_transport_hcidev/tree/main.svg?style=svg)](https://circleci.com/gh/blue-heron/blue_heron_transport_hcidev/tree/main)

To use, add `:blue_heron_transport_hcidev` to your `mix.exs` dependencies and
adapt the following to initialize a transport context.

```elixir
config = %BlueHeron.HCI.Transport.HCIDev{
  device: "/dev/ttyACM0",
}
{:ok, ctx} = BlueHeron.transport(config)
```

## License

The source code is released under the MIT license.

Check [LICENSE](LICENSE) files for more information.
