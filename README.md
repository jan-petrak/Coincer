# Coincer

## Terminology
- Host: A device that is possibly connected to the Coincer network
- Identifier: The public key of a cryptographic keypair
- Identity: A peer representation that we own (identifier + its corresponding
  secret key)
- Message trace: A tuple of a peer-to-peer message's nonce and a pointer to a
  neighbour from which we've received the message. The message traces are
  being used for routing loop detection
- n2n: neighbour-to-neighbour
- Neighbour: A host that we are interconnected with
- Nonce: Ever-increasing integer tied to every message sent for message
  repetiton or replay attack detection
- p2p: peer-to-peer
- Peer: The Coincer network participant
