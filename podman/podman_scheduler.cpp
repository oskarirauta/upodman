#include <deque>
#include <iostream>

#include "mutex.hpp"
#include "podman_t.hpp"
#include "podman_scheduler.hpp"

Podman::Scheduler::Scheduler(void) {

	this -> podman = nullptr;
	this -> next_task = 0;
}

Podman::Scheduler::Scheduler(Podman::podman_t *podman) {

	this -> podman = podman;
	this -> next_task = 0;
}

Podman::Scheduler::Task Podman::Scheduler::nextTask(void) {

	return (Podman::Scheduler::Task)this -> next_task;
}

Podman::Scheduler::Cmd *Podman::Scheduler::nextCmd(void) {

	if ( this -> cmds.size() == 0 )
		return nullptr;
	else return &this -> cmds[0];
}

void Podman::Scheduler::run(void) {

	if ( this -> podman == nullptr )
		return;

	std::lock_guard<std::mutex> guard(mutex.podman_sched);

	if ( this -> podman -> status != Podman::RUNNING ) {
		this -> podman -> update_system();
		return;
	}

	if ( this -> cmds.size() > 0 ) {

		if ( this -> cmds[0].name.empty()) {
			this -> cmds.pop_front();
			return;
		}

		switch ( this -> cmds[0].type ) {
			case Podman::Scheduler::CmdType::CONTAINER_STOP:
				this -> podman -> container_stop(this -> cmds[0].name);
				break;
			case Podman::Scheduler::CmdType::CONTAINER_START:
				this -> podman -> container_start(this -> cmds[0].name);
				break;
			case Podman::Scheduler::CmdType::CONTAINER_RESTART:
				this -> podman -> container_restart(this -> cmds[0].name);
				break;
			case Podman::Scheduler::CmdType::POD_STOP:
				this -> podman -> pod_stop(this -> cmds[0].name);
				break;
			case Podman::Scheduler::CmdType::POD_START:
				this -> podman -> pod_start(this -> cmds[0].name);
				break;
			case Podman::Scheduler::CmdType::POD_RESTART:
				this -> podman -> pod_restart(this -> cmds[0].name);
				break;
		}

		this -> cmds.pop_front();

		return;
	}

	Podman::Scheduler::Task task = (Podman::Scheduler::Task)this -> next_task;

	switch ( this -> next_task ) {
		case Podman::Scheduler::Task::UPDATE_CONTAINERS:
			this -> podman -> update_containers();
			break;
		case Podman::Scheduler::Task::UPDATE_PODS:
			this -> podman -> update_pods();
			break;
		case Podman::Scheduler::Task::UPDATE_STATS:
			this -> podman -> update_stats();
			break;
		case Podman::Scheduler::Task::UPDATE_LOGS:
			this -> podman -> update_logs();
			break;
		case Podman::Scheduler::Task::UPDATE_NETWORKS:
			this -> podman -> update_networks();
			break;
		case Podman::Scheduler::Task::UPDATE_BUSY_CYCLES:
			this -> podman -> update_busy_cycles();
			break;
	}

	this -> next_task++;
	if ((Podman::Scheduler::Task)this -> next_task == Podman::Scheduler::Task::__UPDATE_MAX__ )
		this -> next_task = 0;
}

void Podman::Scheduler::addCmd(Podman::Scheduler::Cmd cmd) {

	if ( cmd.name.empty())
		return;

	std::lock_guard<std::mutex> guard(mutex.podman_sched);

	if ( this -> cmds.size() > 0 )
		if ( this -> cmds[this -> cmds.size() - 1].type == cmd.type &&
			this -> cmds[this -> cmds.size() - 1].name == cmd.name )
				return; // No duplicates

	this -> cmds.push_back(cmd);
}
