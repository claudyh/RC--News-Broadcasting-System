## News Broadcasting System
*in C programming language*

&nbsp;

### 📋 Project Description
---

This project involves the development of a news broadcasting system that utilizes TCP/IP protocols for communication. The system will be built in two phases and will include UDP, TCP, and IP multicast communication to manage news dissemination. The network infrastructure consists of three routers and two switches, ensuring connectivity between clients and the server. The application will be tested using Linux-based Docker images within GNS3.

&nbsp;

*Network Setup*

The communication network is structured as follows:
- Three routers (R1, R2, R3)
- Two switches connecting the clients and the server
- NAT (Network Address Translation) implemented on router R3 for private network access
- IPv4 addressing following specific subnet allocation
  
[Structure on: NetworkSetup.png]

&nbsp;

*Phase 1: Network Configuration and Administration Console*

1. Set up the network using GNS3, ensuring communication between all devices.
2. Implement the initial server version, supporting an administration console via UDP.
3. Use netcat for testing client-server communication before full implementation.

&nbsp;

*Phase 2: Full News Service Implementation*

1. Develop the server to authenticate clients and manage news topics.
2. Implement TCP-based communication between clients and the server.
3. Enable topic subscriptions and multicast news reception for subscribed users.
4. Develop an administrative console for managing users and topics.
5. Allow authorized clients to publish news to specific topics.
6. Ensure proper network functionality, including NAT and multicast support.

&nbsp;

*Functionalities*

**Server**
- User Authentication: Login via username and password.
- Topic Management: List, create, and assign users to topics.
- Multicast Communication: Send news to subscribed clients.
- Administrative Console: Manage users via UDP-based CLI.
- Supports concurrent clients using TCP connections.

**Client**
- Reader Mode: List and subscribe to topics & Receive news through multicast groups.
- Journalist Mode: Create new topics & Publish news to subscribed users.

&nbsp;

---

**NOTE:** The project development has been completed, with all functionalities implemented and proper network operation ensured, including NAT and multicast. However, some areas may still contain bugs. This project was developed within the scope of a Computer Science course by Cláudia Torres and Guilherme Rodrigues.
