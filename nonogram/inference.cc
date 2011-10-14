/**
 * inference.cc
 * 
 * Line solver implementation.
 *
 * @author  Edwin Boaz Soenaryo
 * @author  Nguyen Tat Thang
 */

#include "nonogram.h"

#include <cstring>
#include <stdio.h>

using namespace std;


// Prints the line e.g. # clue=[1 2] cells=[_*.__*__.]
inline void CInferenceEngine::DebugPrint()
{
#ifdef _DEBUG
    printf("# clue=[");
    for (std::vector<unsigned int>::const_iterator it=m_vecConst.begin();
         it !=m_vecConst.end(); ++it)
    {
        printf("%d ", *it);
    }
    
    printf("] cells=[");
    for (std::vector<TriState>::const_iterator it= m_vecCells.begin();
         it != m_vecCells.end(); ++it)
    {
        printf("%c", *it);
    }
    printf("]\n");
#endif
}

// Assigns new value to the referenced cell.
// If the cell did not previously have a value, the changed cell list will be updated
// to reflect the presence of new constraint.
// If the new value is different from previous value, there is a contradiction.
inline bool CInferenceEngine::Assign(TriState& cell, TriState newVal, size_t cellIndex)
{
    if (cell != newVal)
    {
        if (cell != ts_dontknow) throw -1; //Contradiction occurred
        cell = newVal;
        m_bSelfChanged = true;
        m_vecChanged[cellIndex] = true;
        return true;
    }
    return false;
}

// Intersects solids and spaces among valid positions
// Note that because this algorithm enumerates all valid positions,
// we do not need to care about which block a solid/space belongs to
// (unlike Wikipedia's Simple Blocks and Simple Spaces).
void CInferenceEngine::Accumulate(int* pos, TriState* accumulator, bool& bFirst)
{
    const int count = m_vecConst.size();
    const int len = m_vecCells.size();
    
    TriState tmp[len];
    for (int i=0; i<len; ++i)
        tmp[i] = ts_false;
    
    for (int i=0; i<count; ++i)
    {
        for (int j=0; j<m_vecConst[i]; ++j)
            tmp[pos[i] + j] = ts_true;
    }
    
    for (int i=0; i<len; ++i)
    {
        if (bFirst)
            accumulator[i] = tmp[i];
        else if (accumulator[i] != tmp[i])
            accumulator[i] = ts_dontknow;
    }
    
    bFirst = false;
}

// Enumerates all possible starting positions for each blocks in the current line.
// This enumeration respects the presence of other blocks in the line and constraints
// (cells which value have been determined)
// When it finds a valid arrangement, it will call Accumulate to intersect these valid positions
void CInferenceEngine::Enumerate(int b, int* pos, TriState* accumulator, bool& bFirst)
{
    if (b == m_vecConst.size())
    {
        //We have a valid set of block positions
        Accumulate(pos, accumulator, bFirst);
        return;
    }
    
    int start = (b == 0) ? 0 : pos[b-1] + m_vecConst[b-1] + 1;
    int prevSpaceStart = (b == 0) ? 0 : pos[b-1] + m_vecConst[b-1];
    
    const int len = m_vecCells.size();
    for (int i=start; i<len; ++i)
    {
        pos[b] = i;
        
        //Between the previous block's end to this block's start,
        //there should not be any constraint which is a solid.
        bool bViolatesSolid = false;
        for (int j=prevSpaceStart; j<i; ++j)
        {
            if (m_vecCells[j] == ts_true)
            {
                bViolatesSolid = true;
                break;
            }
        }
        if (bViolatesSolid) continue;
        
        //From this block's start to this block's end,
        //there should not be any constraint which is a space.
        bool bViolatesSpace = false;
        for (int j=i; j<i+m_vecConst[b]; ++j)
        {
            if (j >= m_vecCells.size() || m_vecCells[j] == ts_false)
            {
                bViolatesSpace = true;
                break;
            }
        }
        if (bViolatesSpace) continue;
        
        //Special handling for the last block
        //After this block's end onwards, there should not be any
        //constraint which is a solid.
        //TODO: Refactor this
        if (b == m_vecConst.size()-1)
        {
            for (int j=i+m_vecConst[b]; j<m_vecCells.size(); ++j)
            {
                if (m_vecCells[j] == ts_true)
                {
                    bViolatesSolid = true;
                    break;
                }
            }
            if (bViolatesSolid) continue;
        }
        
        Enumerate(b+1, pos, accumulator, bFirst);
    }
}

// Wrapper function to run the line solver algorithm.
// Sets up the data structures and assigns back the output.
int CInferenceEngine::Infer()
{
    try
    {
        const int len = m_vecConst.size();
        int* pos = new int[len];
        memset(pos, 0, len * sizeof(int));
    
        const int count = m_vecCells.size();
        TriState* accumulator = new TriState[count];
        for (size_t i=0; i<count; ++i)
            accumulator[i] = ts_dontknow;
        
        bool bFirst = true;
        Enumerate(0, pos, accumulator, bFirst);
        
        //Save the inference output
        for (size_t i=0; i<count; ++i)
        {
            if (accumulator[i] != ts_dontknow)
                Assign(m_vecCells[i], accumulator[i], i);
        }
    
        DebugPrint();
        return 0;
    } catch (...) { return -1; } //Contradiction
}
