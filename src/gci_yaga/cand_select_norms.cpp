#include "gci_yaga_subroutines.h"

void cand_select_precalc_norms(const valarray& u, const candvec& cands, valarray& norms, double& maxNorm)
{
    const size_t Ncand(cands.size());

    size_t k, p, q;
    double nrm;

    maxNorm = -HUGE_VAL;

    k = 0;
    p = cands[k].first;

    while (k < Ncand - 1)
    {
        q = cands[k+1].first;
        
        valarray seg(u[std::slice(p, q - p, 1)]);

        nrm = sqrt((seg * seg).sum());
        if (nrm > maxNorm) {
            maxNorm = nrm;
        }

        norms[k] = nrm;

        p = q;
        k++;
    }
    
    
}
