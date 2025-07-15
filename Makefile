all: serverC.cpp serverEE.cpp serverCS.cpp serverM.cpp client.cpp 
	g++ -o serverC serverC.cpp --std=c++11

	g++ -o serverEE serverEE.cpp --std=c++11

	g++ -o serverCS serverCS.cpp --std=c++11
	
	g++ -o serverM serverM.cpp --std=c++11

	g++ -o client client.cpp --std=c++11



.PHONY: serverC
serverC:
	./serverC

.PHONY: serverEE
serverT:
	./serverEE

.PHONY: serverCS
serverS:
	./serverCS

.PHONY: serverM
serverP:
	./serverM

.PHONY: client
clientA:
	./client


