#ifndef __CPUTKEYFRAMEANIMATION_H__
#define __CPUTKEYFRAMEANIMATION_H__
#include "CPUTBase.h"
#include "CPUTMatrix.h"
#include "CPUTServicesWin.h"

#include <vector> // std::vector
#include "..\\..\\CPRT\\CPRT File Loader\\iAsset.h"

class CPUTKeyframeAnimation
{
public:
    // Animation 
    // Keyframe animation functions
    bool HasKeyframeAnimation();

    // todo: is there channel names 
    CPUTResult GetChannelName(int channel, char* pName);

    // todo: load!
    CPUTResult LoadKeyframeAnimation(const char* pFilename);

    // get's the time stamp of the very last animation in this channel
    float GetLastKeyframeTime( unsigned int animationChannel = 0 );

    // makes a local copy of this channel's animations
    void SetKeyframeAnimation(std::vector<IntelAsset::iAnimationAsset<>::KeyFrame<> > &frames, unsigned int animationChannel = -1);

    // get the keyframe animation transformation
    void GetKeyframeAnimationTransform(const int frame, CPUTMatrix* pMatrix, unsigned int animationChannel = 0);
    void GetKeyframeAnimationTransform(double fTime, CPUTMatrix* pMatrix, unsigned int animationChannel = 0);

	// get framesplit (equivalent to current frame)
	int GetCurrentFrameSplit(unsigned int animationChannel = 0) const	{	return m_frameSplit.at(animationChannel); }
	void IncrementCurrentFrameSplit(unsigned int animationChannel = 0) 
	{
		if( animationChannel < m_frameSplit.size() )
		{
			++m_frameSplit[animationChannel];
			if(m_frameSplit[animationChannel] == m_KeyframeAnimations[animationChannel].size())
			{
				m_frameSplit[animationChannel] = 0;
			}
		}
	}
	void DecrementCurrentFrameSplit(unsigned int animationChannel = 0)
	{
		if( animationChannel < m_frameSplit.size() )
		{
			--m_frameSplit[animationChannel];
			if(m_frameSplit[animationChannel] < 0)
			{
				m_frameSplit[animationChannel] = (int) m_KeyframeAnimations[animationChannel].size() - 1;
			}
		}
	}

protected:
    // keyframe animation
	std::vector<int> m_frameSplit;
	std::vector< std::vector<IntelAsset::iAnimationAsset<>::KeyFrame<> > > m_KeyframeAnimations;

    // helper functions
    float LERP(	const float &timeBottom, 
				const float &timeTop, 
				const float &timeInTheMiddle, 
				const float &bottomTransform,
				const float &topTransform);

	void SLERP(	const float &timeBottom, 
				const float &timeTop, 
				const float &timeInTheMiddle, 
				const float bottomTransform[], 
				const float topTransform[], 
				float result[]);
};

#endif //#ifndef __CPUTKEYFRAMEANIMATION_H__