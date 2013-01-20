#include "CPUTKeyframeAnimation.h"

// Animation 

// Keyframe Animation

// Does this mesh have an animation associated with it?
//-----------------------------------------------------------------------------
bool CPUTKeyframeAnimation::HasKeyframeAnimation()	
{	
    return m_KeyframeAnimations.size() > 0; 
}



// get's the time stamp of the very last animation in this channel
//-----------------------------------------------------------------------------
float CPUTKeyframeAnimation::GetLastKeyframeTime( unsigned int animationChannel)
{
    if( animationChannel >= m_KeyframeAnimations.size() )//out of bounds protection
        return 0.0f;
    return m_KeyframeAnimations[animationChannel].back().time;
}

/*
// redudant?
// transform the model based on the frame number you give
// this simply updates the ModelTransform matrix to the right
// place.  It will then also transform each mesh object below
// it as well.
//-----------------------------------------------------------------------------
void CPUTKeyframeAnimation::TransformMesh(const double fTime, unsigned int animationChannel) 	
{
    if( animationChannel >= m_KeyframeAnimations.size() )//out of bounds protection
        return;

    float time = (float)fTime;
    if( m_KeyframeAnimations[animationChannel].back().time < fTime )
    {
        float integral;
        time = modf((float)fTime / m_KeyframeAnimations[animationChannel].back().time, &integral);
        time *= m_KeyframeAnimations[animationChannel].back().time;
    }

    unsigned int count = 0;
    for( ;  count < m_KeyframeAnimations[ animationChannel ].size() ; ( (unsigned int)m_frameSplit[animationChannel] < m_KeyframeAnimations[animationChannel].size() - 1) ? ++m_frameSplit[animationChannel] : 0 , ++count )
    {
        if ( ( m_KeyframeAnimations[ animationChannel ][ m_frameSplit [ animationChannel ] ].time > time ) 
            && ( ( ( m_frameSplit[animationChannel] - 1 < 0 ) ? m_KeyframeAnimations[animationChannel][0].time :  m_KeyframeAnimations[animationChannel][m_frameSplit[animationChannel]-1].time ) <= time ) )
        {
            break;
        }
    }
    m_frameSplit[animationChannel] = ( count == m_KeyframeAnimations[ animationChannel ].size() ) ? 0 : m_frameSplit[animationChannel];

    CPUTMatrix newTransform;
    for(int x = 0; x < 4; ++x)
    {
        for(int y = 0; y < 4; ++y)
        {

            newTransform.m_pData[x][y] =	LERP(	( m_frameSplit[animationChannel] - 1 < 0 ) ? m_KeyframeAnimations[animationChannel][0].time :  m_KeyframeAnimations[animationChannel][m_frameSplit[animationChannel]-1].time,
                m_KeyframeAnimations[animationChannel][m_frameSplit[animationChannel]].time,
                time,
                ( m_frameSplit[animationChannel] - 1 < 0 ) ? m_KeyframeAnimations[animationChannel][0].transform._4x4[x][y] :  m_KeyframeAnimations[animationChannel][m_frameSplit[animationChannel]-1].transform._4x4[x][y],
                m_KeyframeAnimations[animationChannel][m_frameSplit[animationChannel]].transform._4x4[x][y]	);
        }
    }
    SetMeshTransform(&newTransform);
}
*/

// stores a local copy of this channel's animations
//-----------------------------------------------------------------------------
void CPUTKeyframeAnimation::SetKeyframeAnimation(std::vector<IntelAsset::iAnimationAsset<>::KeyFrame<> > &frames, unsigned int animationChannel )	
{
    // if this channel is -1, then we set this as the 'default' or only channel
    if( animationChannel == -1 )
    {
        // store the channel in our list of animations for this model
        m_KeyframeAnimations.push_back(frames); 
        m_frameSplit.push_back(0);

        // todo: does this cause timing problems?
        // reset the timestamps to be 0.0f time-based (cut out the original start time)
        if ( m_KeyframeAnimations.back().front().time != 0.0f )
        {
            float offset = ( m_KeyframeAnimations.back().front().time > 0.0f ) ? -m_KeyframeAnimations.back().front().time : m_KeyframeAnimations.back().front().time;
            for(std::vector<IntelAsset::iAnimationAsset<>::KeyFrame<> >::iterator frame = m_KeyframeAnimations.back().begin();
                frame != m_KeyframeAnimations.back().end();
                ++frame)
            {
                (*frame).time += offset;
            }
        }
    }
    else
    {
        if ( animationChannel < m_KeyframeAnimations.size() )
        {                     
            // over-write the existing animation at this slot (if any) and insert
            // this channel in the correct position
            m_KeyframeAnimations[animationChannel] = frames;
            m_frameSplit[animationChannel] = 0;

            // todo: does this cause timing problems?
            // reset the timestamps to be 0.0f time-based (cut out the original start time)
            if ( m_KeyframeAnimations[animationChannel].front().time != 0.0f )
            {
                float offset = ( m_KeyframeAnimations[animationChannel].front().time > 0.0f ) ? -m_KeyframeAnimations[animationChannel].front().time : m_KeyframeAnimations[animationChannel].front().time;
                for(std::vector<IntelAsset::iAnimationAsset<>::KeyFrame<> >::iterator frame = m_KeyframeAnimations[animationChannel].begin();
                    frame != m_KeyframeAnimations[animationChannel].end();
                    ++frame)
                {
                    (*frame).time += offset;
                }
            }
        }
    }
}

// todo: is there channel names 
//CPUResult GetChannelName(int channel, char* pName);

// Get the transform for this object at this the specified frame
//-----------------------------------------------------------------------------
void CPUTKeyframeAnimation::GetKeyframeAnimationTransform(const int frame, CPUTMatrix* pMatrix, unsigned int animationChannel)
{
    if( animationChannel >= m_KeyframeAnimations.size() || !pMatrix )//out of bounds protection and bad pointer
        return;
     
	pMatrix->m_pData[0][3] = m_KeyframeAnimations[animationChannel][m_frameSplit[animationChannel]].translation[0];
	pMatrix->m_pData[1][3] = m_KeyframeAnimations[animationChannel][m_frameSplit[animationChannel]].translation[1];
	pMatrix->m_pData[2][3] = m_KeyframeAnimations[animationChannel][m_frameSplit[animationChannel]].translation[2];
	pMatrix->m_pData[3][3] = 1.0f;
        
	pMatrix->m_pData[0][0] = m_KeyframeAnimations[animationChannel][m_frameSplit[animationChannel]].scale[0] * ( m_KeyframeAnimations[animationChannel][m_frameSplit[animationChannel]].qRotation[3]*m_KeyframeAnimations[animationChannel][m_frameSplit[animationChannel]].qRotation[3] + m_KeyframeAnimations[animationChannel][m_frameSplit[animationChannel]].qRotation[0]*m_KeyframeAnimations[animationChannel][m_frameSplit[animationChannel]].qRotation[0] - m_KeyframeAnimations[animationChannel][m_frameSplit[animationChannel]].qRotation[1]*m_KeyframeAnimations[animationChannel][m_frameSplit[animationChannel]].qRotation[1] - m_KeyframeAnimations[animationChannel][m_frameSplit[animationChannel]].qRotation[2]*m_KeyframeAnimations[animationChannel][m_frameSplit[animationChannel]].qRotation[2] );
	pMatrix->m_pData[0][1] = ( 2 * m_KeyframeAnimations[animationChannel][m_frameSplit[animationChannel]].qRotation[0]*m_KeyframeAnimations[animationChannel][m_frameSplit[animationChannel]].qRotation[1] - 2 * m_KeyframeAnimations[animationChannel][m_frameSplit[animationChannel]].qRotation[3]*m_KeyframeAnimations[animationChannel][m_frameSplit[animationChannel]].qRotation[2] );
	pMatrix->m_pData[0][2] = ( 2 * m_KeyframeAnimations[animationChannel][m_frameSplit[animationChannel]].qRotation[0]*m_KeyframeAnimations[animationChannel][m_frameSplit[animationChannel]].qRotation[3] + 2 * m_KeyframeAnimations[animationChannel][m_frameSplit[animationChannel]].qRotation[3]*m_KeyframeAnimations[animationChannel][m_frameSplit[animationChannel]].qRotation[1] );
	pMatrix->m_pData[1][0] = ( 2 * m_KeyframeAnimations[animationChannel][m_frameSplit[animationChannel]].qRotation[0]*m_KeyframeAnimations[animationChannel][m_frameSplit[animationChannel]].qRotation[1] + 2 * m_KeyframeAnimations[animationChannel][m_frameSplit[animationChannel]].qRotation[3]*m_KeyframeAnimations[animationChannel][m_frameSplit[animationChannel]].qRotation[2] );
	pMatrix->m_pData[1][1] = m_KeyframeAnimations[animationChannel][m_frameSplit[animationChannel]].scale[1] * ( m_KeyframeAnimations[animationChannel][m_frameSplit[animationChannel]].qRotation[3]*m_KeyframeAnimations[animationChannel][m_frameSplit[animationChannel]].qRotation[3] - m_KeyframeAnimations[animationChannel][m_frameSplit[animationChannel]].qRotation[0]*m_KeyframeAnimations[animationChannel][m_frameSplit[animationChannel]].qRotation[0] + m_KeyframeAnimations[animationChannel][m_frameSplit[animationChannel]].qRotation[1]*m_KeyframeAnimations[animationChannel][m_frameSplit[animationChannel]].qRotation[1] - m_KeyframeAnimations[animationChannel][m_frameSplit[animationChannel]].qRotation[2]*m_KeyframeAnimations[animationChannel][m_frameSplit[animationChannel]].qRotation[2] );
	pMatrix->m_pData[1][2] = ( 2 * m_KeyframeAnimations[animationChannel][m_frameSplit[animationChannel]].qRotation[1]*m_KeyframeAnimations[animationChannel][m_frameSplit[animationChannel]].qRotation[2] - 2 * m_KeyframeAnimations[animationChannel][m_frameSplit[animationChannel]].qRotation[3]*m_KeyframeAnimations[animationChannel][m_frameSplit[animationChannel]].qRotation[0] );
	pMatrix->m_pData[2][0] = ( 2 * m_KeyframeAnimations[animationChannel][m_frameSplit[animationChannel]].qRotation[0]*m_KeyframeAnimations[animationChannel][m_frameSplit[animationChannel]].qRotation[2] - 2 * m_KeyframeAnimations[animationChannel][m_frameSplit[animationChannel]].qRotation[3]*m_KeyframeAnimations[animationChannel][m_frameSplit[animationChannel]].qRotation[1] );
	pMatrix->m_pData[2][1] = ( 2 * m_KeyframeAnimations[animationChannel][m_frameSplit[animationChannel]].qRotation[1]*m_KeyframeAnimations[animationChannel][m_frameSplit[animationChannel]].qRotation[2] + 2 * m_KeyframeAnimations[animationChannel][m_frameSplit[animationChannel]].qRotation[3]*m_KeyframeAnimations[animationChannel][m_frameSplit[animationChannel]].qRotation[0] );
	pMatrix->m_pData[2][2] = m_KeyframeAnimations[animationChannel][m_frameSplit[animationChannel]].scale[2] * ( m_KeyframeAnimations[animationChannel][m_frameSplit[animationChannel]].qRotation[3]*m_KeyframeAnimations[animationChannel][m_frameSplit[animationChannel]].qRotation[3] - m_KeyframeAnimations[animationChannel][m_frameSplit[animationChannel]].qRotation[0]*m_KeyframeAnimations[animationChannel][m_frameSplit[animationChannel]].qRotation[0] - m_KeyframeAnimations[animationChannel][m_frameSplit[animationChannel]].qRotation[1]*m_KeyframeAnimations[animationChannel][m_frameSplit[animationChannel]].qRotation[1] + m_KeyframeAnimations[animationChannel][m_frameSplit[animationChannel]].qRotation[2]*m_KeyframeAnimations[animationChannel][m_frameSplit[animationChannel]].qRotation[2] );

	pMatrix->m_pData[3][0] = 0.0f;
	pMatrix->m_pData[3][1] = 0.0f;
	pMatrix->m_pData[3][2] = 0.0f;


	//transpose
	float temp;
	for( int i = 0; i <= 2; ++i )
	{
		for( int j = i+1; j <= 3; ++j )
		{
			temp = pMatrix->m_pData[i][j];
			pMatrix->m_pData[i][j] = pMatrix->m_pData[j][i];
			pMatrix->m_pData[j][i] = temp;
		}
	}
        
    
}

// Get the transform for this object at this the specified time
// performing a simple lerp if we're between frames
//-----------------------------------------------------------------------------
void CPUTKeyframeAnimation::GetKeyframeAnimationTransform(double fTime, CPUTMatrix* pMatrix, unsigned int animationChannel )
{
    if( animationChannel >= m_KeyframeAnimations[animationChannel].size() || !pMatrix )//out of bounds protection
    {
        // todo: potentially return identity matrix?
        return;
    }

    float time = (float)fTime;
    if( m_KeyframeAnimations[animationChannel].back().time < fTime )
    {
        float integral;
        time = modf((float)fTime / m_KeyframeAnimations[animationChannel].back().time, &integral);
        time *= m_KeyframeAnimations[animationChannel].back().time;
    }

    unsigned int count = 0;
    for( ;  count < m_KeyframeAnimations[ animationChannel ].size() ; ( (unsigned int)m_frameSplit[animationChannel] < m_KeyframeAnimations[animationChannel].size() - 1) ? ++m_frameSplit[animationChannel] : 0 , ++count )
    {
        if ( ( m_KeyframeAnimations[ animationChannel ][ m_frameSplit [ animationChannel ] ].time > time ) 
            && ( ( ( m_frameSplit[animationChannel] - 1 < 0 ) ? m_KeyframeAnimations[animationChannel][0].time :  m_KeyframeAnimations[animationChannel][m_frameSplit[animationChannel]-1].time ) <= time ) )
        {
            break;
        }
    }

    m_frameSplit[animationChannel] = ( count == m_KeyframeAnimations[ animationChannel ].size() ) ? 0 : m_frameSplit[animationChannel];

	float scale[3] = { 1.0f, 1.0f, 1.0f };
	float quatResult[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	for(int y = 0; y < 4; ++y)
    {
		pMatrix->m_pData[y][0] = 0.0f;
		pMatrix->m_pData[y][1] = 0.0f;
		pMatrix->m_pData[y][2] = 0.0f;
		//LERP translation
        pMatrix->m_pData[y][3] =	LERP(	( m_frameSplit[animationChannel] - 1 < 0 ) ? m_KeyframeAnimations[animationChannel][0].time :  m_KeyframeAnimations[animationChannel][m_frameSplit[animationChannel]-1].time,
											m_KeyframeAnimations[animationChannel][m_frameSplit[animationChannel]].time,
											time,
											( m_frameSplit[animationChannel] - 1 < 0 ) ? m_KeyframeAnimations[animationChannel][0].translation[y] :  m_KeyframeAnimations[animationChannel][m_frameSplit[animationChannel]-1].translation[y],
											m_KeyframeAnimations[animationChannel][m_frameSplit[animationChannel]].translation[y]	);
		//LERP scale
		if ( y < 3 )
		{
			scale[y] =	LERP(	( m_frameSplit[animationChannel] - 1 < 0 ) ? m_KeyframeAnimations[animationChannel][0].time :  m_KeyframeAnimations[animationChannel][m_frameSplit[animationChannel]-1].time,
								m_KeyframeAnimations[animationChannel][m_frameSplit[animationChannel]].time,
								time,
								( m_frameSplit[animationChannel] - 1 < 0 ) ? m_KeyframeAnimations[animationChannel][0].scale[y] :  m_KeyframeAnimations[animationChannel][m_frameSplit[animationChannel]-1].scale[y],
								m_KeyframeAnimations[animationChannel][m_frameSplit[animationChannel]].scale[y]	);
		}
    }

	//SLERP the quaternion
	SLERP(	( m_frameSplit[animationChannel] - 1 < 0 ) ? m_KeyframeAnimations[animationChannel][0].time :  m_KeyframeAnimations[animationChannel][m_frameSplit[animationChannel]-1].time,
            m_KeyframeAnimations[animationChannel][m_frameSplit[animationChannel]].time,
            time,
            ( m_frameSplit[animationChannel] - 1 < 0 ) ? m_KeyframeAnimations[animationChannel][0].qRotation :  m_KeyframeAnimations[animationChannel][m_frameSplit[animationChannel]-1].qRotation,
            m_KeyframeAnimations[animationChannel][m_frameSplit[animationChannel]].qRotation,
			quatResult );

	//http://en.wikipedia.org/wiki/Quaternions_and_spatial_rotation
	pMatrix->m_pData[0][0] = scale[0] * ( quatResult[3]*quatResult[3] + quatResult[0]*quatResult[0] - quatResult[1]*quatResult[1] - quatResult[2]*quatResult[2] );
	pMatrix->m_pData[0][1] = ( 2 * quatResult[0]*quatResult[1] - 2 * quatResult[3]*quatResult[2] );
	pMatrix->m_pData[0][2] = ( 2 * quatResult[0]*quatResult[3] + 2 * quatResult[3]*quatResult[1] );
	pMatrix->m_pData[1][0] = ( 2 * quatResult[0]*quatResult[1] + 2 * quatResult[3]*quatResult[2] );
	pMatrix->m_pData[1][1] = scale[1] * ( quatResult[3]*quatResult[3] - quatResult[0]*quatResult[0] + quatResult[1]*quatResult[1] - quatResult[2]*quatResult[2] );
	pMatrix->m_pData[1][2] = ( 2 * quatResult[1]*quatResult[2] - 2 * quatResult[3]*quatResult[0] );
	pMatrix->m_pData[2][0] = ( 2 * quatResult[0]*quatResult[2] - 2 * quatResult[3]*quatResult[1] );
	pMatrix->m_pData[2][1] = ( 2 * quatResult[1]*quatResult[2] + 2 * quatResult[3]*quatResult[0] );
	pMatrix->m_pData[2][2] = scale[2] * ( quatResult[3]*quatResult[3] - quatResult[0]*quatResult[0] - quatResult[1]*quatResult[1] + quatResult[2]*quatResult[2] );

	pMatrix->m_pData[3][3] = 1.0f;

	//transpose
	for( int i = 0; i <= 2; ++i )
	{
		for( int j = i+1; j <= 3; ++j )
		{
			time = pMatrix->m_pData[i][j];
			pMatrix->m_pData[i][j] = pMatrix->m_pData[j][i];
			pMatrix->m_pData[j][i] = time;
		}
	}
}

// lerp function
//-----------------------------------------------------------------------------
float CPUTKeyframeAnimation::LERP(const float &timeBottom,const float &timeTop,const float &timeInTheMiddle,const float &bottomTransform,const float &topTransform)
{
    if ( timeTop - timeBottom )
    {
        return	bottomTransform + 
            ( (timeInTheMiddle - timeBottom) * topTransform - ( timeInTheMiddle - timeBottom ) * bottomTransform  ) /
            ( timeTop - timeBottom );
    }
    return 0.0f;
}

// lerp function
//-----------------------------------------------------------------------------
void CPUTKeyframeAnimation::SLERP(	const float &timeBottom, 
									const float &timeTop, 
									const float &timeInTheMiddle, 
									const float bottomTransform[], 
									const float topTransform[], 
									float result[])
{
	float parameter = 0.0f;
	float norm = 0.0f;
    if ( timeTop - timeBottom )
    {
		parameter =  timeInTheMiddle - timeBottom / (timeTop - timeBottom);
     ////////The following came from http://stackoverflow.com/questions/4099369/interpolate-between-rotation-matrices/4099423#4099423
		float cosHalfTheta = bottomTransform[3] * topTransform[3] + bottomTransform[0] * topTransform[0] + bottomTransform[1] * topTransform[1] + bottomTransform[2] * topTransform[2];
		// if qa=qb or qa=-qb then theta = 0 and we can return qa
		if (abs(cosHalfTheta) >= 1.0)
		{
			result[3] = bottomTransform[3];
			result[0] = bottomTransform[0];
			result[1] = bottomTransform[1];
			result[2] = bottomTransform[2];
			return;
		}
		// Calculate temporary values.
		float halfTheta = acos(cosHalfTheta);
		float sinHalfTheta = sqrt(1.0f - cosHalfTheta*cosHalfTheta);
		// if theta = 180 degrees then result is not fully defined
		// we could rotate around any axis normal to qa or qb
		if (fabs(sinHalfTheta) < 0.001f)
		{ // fabs is floating point absolute
        
			result[0] = (bottomTransform[0] * 0.5f + topTransform[0] * 0.5f);
			result[1] = (bottomTransform[1] * 0.5f + topTransform[1] * 0.5f);
			result[2] = (bottomTransform[2] * 0.5f + topTransform[2] * 0.5f);
			result[3] = (bottomTransform[3] * 0.5f + topTransform[3] * 0.5f);
			
			norm = sqrt(result[0]*result[0] + result[1]*result[1] + result[2]*result[2] + result[3]*result[3]);
			if(norm)
			{
				result[0] /= norm;
				result[1] /= norm;
				result[2] /= norm;
				result[3] /= norm;
			}
			return;
		}
		float ratioA = sin((1 - parameter) * halfTheta) / sinHalfTheta;
		float ratioB = sin(parameter * halfTheta) / sinHalfTheta; 
		//calculate Quaternion.
    
		result[0] = (bottomTransform[0] * ratioA + topTransform[0] * ratioB);
		result[1] = (ratioA < 0.0f) ? -1.0f*(bottomTransform[1] * ratioA + topTransform[1] * ratioB) : (bottomTransform[1] * ratioA + topTransform[1] * ratioB) ;
		result[2] = (bottomTransform[2] * ratioA + topTransform[2] * ratioB);
		result[3] = (bottomTransform[3] * ratioA + topTransform[3] * ratioB);

		norm = sqrt(result[0]*result[0] + result[1]*result[1] + result[2]*result[2] + result[3]*result[3]);
		if(norm)
		{
			result[0] /= norm;
			result[1] /= norm;
			result[2] /= norm;
			result[3] /= norm;
		}
    }
}