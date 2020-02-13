### Buzz - Distributed microblogging platform

Buzz built on the top of the Qbit technology as DApp.

## Architecture

To start using Buzz DApp user _must_ own a little amount of qbits. Starting Buzz client app for the first time she/he can send a request to the network make a little donation to his primary qbit address.

Each microblog starts with the special entity type - transaction TxBuzzer. Buzzer - contains attributes:
 - @name - human-readable buzzer unique name
 - full_name - human-readable buzzer name (could be non-unique)
 - avatar_picture - small picture in png format (120x120)
 - in:
   - qbit-fee
 - out:
   - miner qbit-fee

To start write micro-posts user need use qbits - tx payment for the each post. Each post - special transaction TxBuzz with attributes:
 - body - micro-blog body in multibyte format
 - in:
   - tx-buzzer
   - qbit-fee
 - out:
   - 
