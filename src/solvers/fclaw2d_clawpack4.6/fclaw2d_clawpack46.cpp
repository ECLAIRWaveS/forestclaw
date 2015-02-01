/*
Copyright (c) 2012 Carsten Burstedde, Donna Calhoun
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "amr_includes.H"

#include "amr_forestclaw.H"
#include "amr_utils.H"
#include "clawpack_fort.H"
#include "fclaw2d_solvers.H"
#include "fclaw2d_clawpack46.H"
#include "fclaw_options.h"



static fc2d_clawpack46_options_t*
get_options(fclaw2d_domain_t* domain)
{
    fclaw2d_domain_data_t* ddata = get_domain_data(domain);
    fc2d_clawpack46_options_t *clawpack_options =
        (fc2d_clawpack46_options_t*) ddata->clawpack_parms;
    return clawpack_options;
}

static fc2d_clawpack46_patch_data_t*
get_patch_data(ClawPatch *cp)
{
    fc2d_clawpack46_patch_data_t *wp =
        (fc2d_clawpack46_patch_data_t*) cp->clawpack_patch_data();
    return wp;
}

/* -----------------------------------------------------------
   Public interface to routines in this file
   ----------------------------------------------------------- */
void fc2d_clawpack46_set_options(fclaw2d_domain_t* domain,
                                 fc2d_clawpack46_options_t* clawopt)
{
    fclaw2d_domain_data_t* ddata = get_domain_data(domain);
    ddata->clawpack_parms = (void*) clawopt;
}

void fc2d_clawpack46_define_auxarray(fclaw2d_domain_t* domain, ClawPatch *cp)
{
    const amr_options_t *gparms = get_domain_parms(domain);
    int mx = gparms->mx;
    int my = gparms->my;
    int mbc = gparms->mbc;

    fc2d_clawpack46_options_t *clawpack_options = get_options(domain);
    int maux = clawpack_options->maux;

    // Construct an index box to hold the aux array
    int ll[2], ur[2];
    ll[0] = 1-mbc;
    ll[1] = 1-mbc;
    ur[0] = mx + mbc;
    ur[1] = my + mbc;    Box box(ll,ur);

    // get solver specific data stored on this patch
    fc2d_clawpack46_patch_data_t *clawpack_patch_data = get_patch_data(cp);
    clawpack_patch_data->auxarray.define(box,maux);
    clawpack_patch_data->maux = maux; // set maux in solver specific patch data (for completeness)
}

void fc2d_clawpack46_get_auxarray(fclaw2d_domain_t* domain,
                                  ClawPatch *cp, double **aux, int* maux)
{
    fc2d_clawpack46_options_t* clawpack_options = get_options(domain);
    *maux = clawpack_options->maux;

    fc2d_clawpack46_patch_data_t *clawpack_patch_data = get_patch_data(cp);
    *aux = clawpack_patch_data->auxarray.dataPtr();
}

void fc2d_clawpack46_setprob(fclaw2d_domain_t* domain)
{
    setprob_();
}


/* This should only be called when a new ClawPatch is created. */
void fc2d_clawpack46_setaux(fclaw2d_domain_t *domain,
                            fclaw2d_patch_t *this_patch,
                            int this_block_idx,
                            int this_patch_idx)
{
    /* ----------------------------------------------------------- */
    // Global parameters
    const amr_options_t *gparms = get_domain_parms(domain);
    int mx = gparms->mx;
    int my = gparms->my;
    int mbc = gparms->mbc;

    /* ----------------------------------------------------------- */
    // Patch specific parameters
    ClawPatch *cp = get_clawpatch(this_patch);
    double xlower = cp->xlower();
    double ylower = cp->ylower();
    double dx = cp->dx();
    double dy = cp->dy();

    /* ----------------------------------------------------------- */
    fc2d_clawpack46_define_auxarray(domain,cp);

    /* ----------------------------------------------------------- */
    // Pointers needed to pass to class setaux call, and other setaux
    // specific arguments
    double *aux;
    int maux;
    fc2d_clawpack46_get_auxarray(domain,cp,&aux,&maux);

    int maxmx = mx;
    int maxmy = my;

    /* ----------------------------------------------------------- */
    // Classic setaux call
    setaux_(maxmx,maxmy,mbc,mx,my,xlower,ylower,dx,dy,maux,aux);
}

void fc2d_clawpack46_qinit(fclaw2d_domain_t *domain,
                           fclaw2d_patch_t *this_patch,
                           int this_block_idx,
                           int this_patch_idx)
{
    /* ----------------------------------------------------------- */
    // Global parameters
    const amr_options_t *gparms = get_domain_parms(domain);
    int mx = gparms->mx;
    int my = gparms->my;
    int mbc = gparms->mbc;
    int meqn = gparms->meqn;

    /* ----------------------------------------------------------- */
    // Patch specific parameters
    ClawPatch *cp = get_clawpatch(this_patch);
    double xlower = cp->xlower();
    double ylower = cp->ylower();
    double dx = cp->dx();
    double dy = cp->dy();

    /* ------------------------------------------------------- */
    // Pointers needed to pass to Fortran
    double* q = cp->q();

    double *aux;
    int maux;
    fc2d_clawpack46_get_auxarray(domain,cp,&aux,&maux);

    // Other input arguments
    int maxmx = mx;
    int maxmy = my;

    /* ------------------------------------------------------- */
    // Call to classic Clawpack 'qinit' routine.
    CLAWPACK_SET_BLOCK(&this_block_idx);
    qinit_(maxmx,maxmy,meqn,mbc,mx,my,xlower,ylower,dx,dy,q,maux,aux);
}

void fc2d_clawpack46_b4step2(fclaw2d_domain_t *domain,
                             fclaw2d_patch_t *this_patch,
                             int this_block_idx,
                             int this_patch_idx,
                             double t,
                             double dt)
{
    /* ----------------------------------------------------------- */
    // Global parameters
    const amr_options_t *gparms = get_domain_parms(domain);
    int mx = gparms->mx;
    int my = gparms->my;
    int mbc = gparms->mbc;
    int meqn = gparms->meqn;

    /* ----------------------------------------------------------- */
    // Patch specific parameters
    ClawPatch *cp = get_clawpatch(this_patch);
    double xlower = cp->xlower();
    double ylower = cp->ylower();
    double dx = cp->dx();
    double dy = cp->dy();

    /* ------------------------------------------------------- */
    // Pointers needed to pass to Fortran
    double* q = cp->q();

    double *aux;
    int maux;
    fc2d_clawpack46_get_auxarray(domain,cp,&aux,&maux);

    // Other input arguments
    int maxmx = mx;
    int maxmy = my;

    /* ------------------------------------------------------- */
    // Classic call to b4step2(..)
    CLAWPACK_SET_BLOCK(&this_block_idx);
    b4step2_(maxmx,maxmy,mbc,mx,my,meqn,q,xlower,ylower,dx,dy,t,dt,maux,aux);
}

void fc2d_clawpack46_src2(fclaw2d_domain_t *domain,
                          fclaw2d_patch_t *this_patch,
                          int this_block_idx,
                          int this_patch_idx,
                          double t,
                          double dt)
{
    /* ----------------------------------------------------------- */
    // Global parameters
    const amr_options_t *gparms = get_domain_parms(domain);
    int mx = gparms->mx;
    int my = gparms->my;
    int mbc = gparms->mbc;
    int meqn = gparms->meqn;

    /* ----------------------------------------------------------- */
    // Patch specific parameters
    ClawPatch *cp = get_clawpatch(this_patch);
    double xlower = cp->xlower();
    double ylower = cp->ylower();
    double dx = cp->dx();
    double dy = cp->dy();

    /* ------------------------------------------------------- */
    // Pointers needed to pass to Fortran
    double* q = cp->q();

    double *aux;
    int maux;
    fc2d_clawpack46_get_auxarray(domain,cp,&aux,&maux);

    // Other input arguments
    int maxmx = mx;
    int maxmy = my;

    /* ------------------------------------------------------- */
    // Classic call to src2(..)
    CLAWPACK_SET_BLOCK(&this_block_idx);
    src2_(maxmx,maxmy,meqn,mbc,mx,my,xlower,ylower,dx,dy,q,maux,aux,t,dt);
}


/* Use this to return only the right hand side of the clawpack algorithm */
double fc2d_clawpack46_step2_rhs(fclaw2d_domain_t *domain,
                                 fclaw2d_patch_t *this_patch,
                                 int this_block_idx,
                                 int this_patch_idx,
                                 double t,
                                 double *rhs)
{
    /* This should evaluate the right hand side, but not actually do the update.
       This will be useful in cases where we want to use something other than
       a single step method.  For example, in a RK scheme, one might want to
       call the right hand side to evaluate stages. */
    return 0;
}


void fc2d_clawpack46_bc2(fclaw2d_domain *domain,
                         fclaw2d_patch_t *this_patch,
                         int this_block_idx,
                         int this_patch_idx,
                         double t,
                         double dt,
                         fclaw_bool intersects_phys_bdry[],
                         fclaw_bool time_interp)
{
    /* ----------------------------------------------------------- */
    // Global parameters
    const amr_options_t *gparms = get_domain_parms(domain);
    int mx = gparms->mx;
    int my = gparms->my;
    int mbc = gparms->mbc;
    int meqn = gparms->meqn;

    /* ----------------------------------------------------------- */
    // Patch specific parameters
    ClawPatch *cp = get_clawpatch(this_patch);
    double xlower = cp->xlower();
    double ylower = cp->ylower();
    double dx = cp->dx();
    double dy = cp->dy();

    /* ------------------------------------------------------ */
    // Set up boundary conditions
    fclaw2d_block_t *this_block = &domain->blocks[this_block_idx];
    fclaw2d_block_data_t *bdata = get_block_data(this_block);
    int *block_mthbc = bdata->mthbc;

    /* Set a local copy of mthbc that can be used for a patch. */
    int mthbc[NumFaces];
    for(int i = 0; i < NumFaces; i++)
    {
        if (intersects_phys_bdry[i])
        {
            mthbc[i] = block_mthbc[i];
        }
        else
        {
            mthbc[i] = -1;
        }
    }

    /* ------------------------------------------------------- */

    /*
      We may be imposing boundary conditions on time-interpolated data;
      and is being done just to help with fine grid interpolation.
      In this case, this boundary condition won't be used to update
      anything
    */
    double *q = cp->q_time_sync(time_interp);

    double *aux;
    int maux;
    fc2d_clawpack46_get_auxarray(domain,cp,&aux,&maux);

    // Other input arguments
    int maxmx = mx;
    int maxmy = my;

    /* ------------------------------------------------------- */
    // Classic call to bc2(..)
    CLAWPACK_SET_BLOCK(&this_block_idx);
    bc2_(maxmx,maxmy,meqn,mbc,mx,my,xlower,ylower,dx,dy,q,maux,aux,t,dt,mthbc);
}


/* This is called from the single_step callback. and is of type 'flaw_single_step_t' */
double fc2d_clawpack46_step2(fclaw2d_domain_t *domain,
                             fclaw2d_patch_t *this_patch,
                             int this_block_idx,
                             int this_patch_idx,
                             double t,
                             double dt)
{
    const amr_options_t* gparms                 = get_domain_parms(domain);
    ClawPatch *cp                               = get_clawpatch(this_patch);
    fc2d_clawpack46_options_t* clawpack_options = get_options(domain);

    SET_CORNERS(cp->block_corner_count());

    int level = this_patch->level;

    double* qold = cp->q();

    double *aux;
    int maux;
    fc2d_clawpack46_get_auxarray(domain,cp,&aux,&maux);

    cp->save_current_step();  // Save for time interpolation

    // Global to all patches
    int mx = gparms->mx;
    int my = gparms->my;
    int mbc = gparms->mbc;
    int meqn = gparms->meqn;

    // Specific to the patch
    double xlower = cp->xlower();
    double ylower = cp->ylower();
    double dx = cp->dx();
    double dy = cp->dy();

    // Specific to solver
    int mwaves = clawpack_options->mwaves;

    int maxm = max(mx,my);

    double cflgrid = 0.0;

    int mwork = (maxm+2*mbc)*(12*meqn + (meqn+1)*mwaves + 3*maux + 2);
    double* work = new double[mwork];

    int size = meqn*(mx+2*mbc)*(my+2*mbc);
    double* fp = new double[size];
    double* fm = new double[size];
    double* gp = new double[size];
    double* gm = new double[size];

    // Replace this with a call to "step2" at some point...
    clawpatch2_(maxm, meqn, maux, mbc, clawpack_options->method,
                clawpack_options->mthlim, clawpack_options->mcapa, mwaves,
                mx, my, qold,
                aux, dx, dy, dt, cflgrid, work, mwork, xlower, ylower,
                level,t, fp, fm, gp, gm);

    delete [] fp;
    delete [] fm;
    delete [] gp;
    delete [] gm;

    delete [] work;

    return cflgrid;
}

double fc2d_clawpack46_update(fclaw2d_domain_t *domain,
                              fclaw2d_patch_t *this_patch,
                              int this_block_idx,
                              int this_patch_idx,
                              double t,
                              double dt)
{
    const fc2d_clawpack46_options_t* clawpack_options =
        get_options(domain);

    fc2d_clawpack46_b4step2(domain,
                            this_patch,
                            this_block_idx,
                            this_patch_idx,t,dt);

    double maxcfl = fc2d_clawpack46_step2(domain,
                                          this_patch,
                                          this_block_idx,
                                          this_patch_idx,t,dt);

    if (clawpack_options->src_term > 0)
    {
        fc2d_clawpack46_src2(domain,
                             this_patch,
                             this_block_idx,
                             this_patch_idx,t,dt);
    }
    return maxcfl;
}


static
void patch_data_new(void** wp)
{
    fc2d_clawpack46_patch_data_t* clawpack_patch_data;
    clawpack_patch_data = new fc2d_clawpack46_patch_data_t;
    *wp = (void*) clawpack_patch_data;

    // or?
    // *wp = (void*) FC2D_ALLOC_ZERO (fc2d_clawpack_patch_data_t, 1);
}

static
void patch_data_delete(void **wp)
{
    fc2d_clawpack46_patch_data_t *clawpack_patch_data = (fc2d_clawpack46_patch_data_t*) *wp;
    delete clawpack_patch_data;

    // or?
    // FC2D_FREE(clawpack_patch_data);

    *wp = (void*) NULL;
}


void fc2d_clawpack46_link_to_clawpatch()
{
    /* These are called whenever a new ClawPatch is created. */
    ClawPatch::f_clawpack_patch_data_new    = &patch_data_new;
    ClawPatch::f_clawpack_patch_data_delete = &patch_data_delete;
}

void  fc2d_clawpack46_link_solvers(fclaw2d_domain_t* domain)
{
    const fc2d_clawpack46_options_t* clawpack_options = get_options(domain);

    fclaw2d_solver_functions_t* sf = get_solver_functions(domain);

    sf->use_single_step_update = fclaw_true;
    sf->use_mol_update = fclaw_false;

    if (clawpack_options->maux > 0)
    {
        sf->f_patch_setup          = &fc2d_clawpack46_setaux;
    }
    else
    {
        sf->f_patch_setup          = &amr_dummy_patch_setup;
    }

    sf->f_patch_initialize         = &fc2d_clawpack46_qinit;
    sf->f_patch_physical_bc        = &fc2d_clawpack46_bc2;

    /* Calls b4step2, step2 and src2 */
    sf->f_patch_single_step_update = &fc2d_clawpack46_update;

    /* This is needed so that a ClawPatch knows how to create data for a clawpack solver */
    fc2d_clawpack46_link_to_clawpatch();
}
