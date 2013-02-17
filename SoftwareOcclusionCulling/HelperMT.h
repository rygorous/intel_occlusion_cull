#ifndef HELPERMT_H
#define HELPERMT_H

static inline void GetWorkExtent(unsigned int *pstart, unsigned int *pend, unsigned int taskId, unsigned int taskCount, unsigned int workItemCount)
{
	unsigned int numRemainingItems = workItemCount % taskCount;

	unsigned int numItemsPerTask1 = workItemCount / taskCount + 1;
	unsigned int numItemsPerTask2 = workItemCount / taskCount;

	unsigned int start, end;
	
	if(taskId < numRemainingItems)
	{
		start = taskId * numItemsPerTask1;
		end   = start +  numItemsPerTask1;
	}
	else
	{
		start = (numRemainingItems * numItemsPerTask1) + ((taskId - numRemainingItems) * numItemsPerTask2);
		end   = start +  numItemsPerTask2;
	}

	*pstart = start;
	*pend = end;
}

#endif