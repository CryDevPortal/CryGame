#ifndef __IJOB_H__
#define __IJOB_H__
#pragma once

struct IJob : public _i_reference_target_t
{
	enum JobStatus
	{
		jsStarting,
		jsInProgress,
		jsDone,
		jsFailed,
	};
	//
	IJob() : m_status(jsStarting)
	{
	}
	virtual ~IJob()
	{
	}
	//
	virtual void Do() = 0;
	virtual inline JobStatus GetStatus() const 
	{ 
		return m_status; 
	}
	// means job in progress status doesn't block parent job on this job
	virtual inline bool IsBlocking() const
	{
		return true;
	}
	virtual inline bool IsWorking() const
	{
		return m_status == jsStarting || m_status == jsInProgress;
	}
protected:
	JobStatus m_status;
};

#endif //__IJOB_H__