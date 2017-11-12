agent: agent.cpp
	g++ agent.cpp -o agent

run: agent
	./agent
