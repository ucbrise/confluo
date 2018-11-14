# Network Monitoring and Diagnosis

Network monitoring and diagnosis is an increasingly challenging task for network
operators. Integrating these functionalities within the network stack at the
end-hosts allows efficiently using end-host programmability and resources with
minimal overheads (end-host stack process the incoming packets anyway). However,
achieving this requires tools that support highly concurrent per-packet capture
at line-rate, support for online queries (for monitoring purposes), and offline
queries (for diagnosis purposes). Confluo interface is a natural fit for
building such a tool -- flows, packets headers and header fields at an end-host
map perfectly to Confluo streams, records and attributes.

For details on the design, implementation and evaluation of the network monitoring
and diagnosis tool, please see our [NSDI paper](https://people.eecs.berkeley.edu/~anuragk/papers/confluo.pdf).