#pragma once
/*
 *  Copyright (C) 2024  Brett Terpstra
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef IMAGE_GP_6_SLR_H
#define IMAGE_GP_6_SLR_H

template<typename T, blt::size_t sample_size>
float mean(const std::array<T, sample_size>& data)
{
    T x = 0;
    for (blt::size_t n = 0; n < sample_size; n++)
    {
        x = x + data[n];
    }
    x = x / sample_size;
    return x;
}

// https://github.com/georgemaier/simple-linear-regression/blob/master/slr.cpp
template<typename T, blt::size_t sample_size>
class slr
{
    private:
        T WN1 = 0, WN2 = 0, WN3 = 0, WN4 = 0, Sy = 0, Sx = 0;
    
    public:
        T r = 0, rsquared = 0, alpha = 0, beta = 0, x = 0, y = 0;
        T yhat = 0, ybar = 0, xbar = 0;
        T SSR = 0, SSE = 0, SST = 0;
        T residualSE = 0, residualmax = 0, residualmin = 0, residualmean = 0, t = 0;
        T SEBeta = 0, sample = 0, residuals[sample_size]{};
        
        slr(const std::array<T, sample_size>& datax, const std::array<T, sample_size>& datay)
        {
            //This is the main regression function that is called when a new SLR object is created.
            
            //Calculate means
            sample = sample_size;
            xbar = mean(datax);
            ybar = mean(datay);
            
            //Calculate r correlation
            for (blt::size_t n = 0; n < sample_size; ++n)
            {
                WN1 += (datax[n] - xbar) * (datay[n] - ybar);
                WN2 += pow((datax[n] - xbar), 2);
                WN3 += pow((datay[n] - ybar), 2);
            }
            WN4 = WN2 * WN3;
            r = WN1 / (std::sqrt(WN4));
            
            //Calculate alpha and beta
            Sy = std::sqrt(WN3 / (sample_size - 1));
            Sx = std::sqrt(WN2 / (sample_size - 1));
            beta = r * (Sy / Sx);
            alpha = ybar - beta * xbar;
            
            //Calculate SSR, SSE, R-Squared, residuals
            for (blt::size_t n = 0; n < sample_size; n++)
            {
                yhat = alpha + (beta * datax[n]);
                SSE += std::pow((yhat - ybar), 2);
                SSR += std::pow((datay[n] - yhat), 2);
                residuals[n] = (datay[n] - yhat);
                if (residuals[n] > residualmax)
                    residualmax = residuals[n];
                if (residuals[n] < residualmin)
                    residualmin = residuals[n];
                residualmean += std::fabs(residuals[n]);
            }
            residualmean = (residualmean / sample_size);
            SST = SSR + SSE;
            rsquared = SSE / SST;           //Can also be obtained by r ^ 2 for simple regression (i.e. 1 independent variable)
            
            //Calculate T-test for Beta
            residualSE = std::sqrt(SSR / (sample_size - 2));
            SEBeta = (residualSE / (Sx * std::sqrt(sample_size - 1)));
            t = beta / SEBeta;
        }
};

#endif //IMAGE_GP_6_SLR_H
