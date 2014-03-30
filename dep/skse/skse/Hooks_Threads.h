#pragma once

class TaskDelegate;

void Hooks_Threads_Init(void);
void Hooks_Threads_Commit(void);

namespace TaskInterface
{
	void AddTask(TaskDelegate * task);
}
