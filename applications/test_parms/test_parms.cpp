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

#include <fc2d_clawpack46.H>
#include <fclaw_base.h>
#include <fclaw2d_map.h>
#include <p4est_connectivity.h>
#include <fclaw2d_map_query.h>

#include <amr_forestclaw.H>
#include <amr_utils.H>
#include <fclaw_options.h>




typedef struct user_options
{
    int example;

    const char* frames_string;
    int *frames;
    const char* density_string;
    double *density;

    amr_options_t* gparms;

    int is_registered;

} user_options_t;

static void *
options_register_user (fclaw_app_t * app, void *package, sc_options_t * opt)
{
    user_options_t* user = (user_options_t*) package;

    /* [user] User options */
    /* [user] User options */
    sc_options_add_int (opt, 0, "example", &user->example, 0,
                        "[user] 0 (simple); 1 (simulation)");

    fclaw_options_add_int_array(opt,0,"frames",&user->frames_string,
                                NULL, &user->frames,0,
                                "[user] Frames these frames [NULL]");

    fclaw_options_add_double_array(opt,0,"density",&user->density_string,
                                   NULL,&user->density, 0,
                                   "[user] Density [NULL]");

    user->is_registered = 1;
    return NULL;
}

static fclaw_exit_type_t
options_postprocess_user (fclaw_app_t * a, void *package, void *registered)
{
    user_options_t* user = (user_options_t*) package;

    if (user->example == 2)
    {
        fclaw_options_convert_int_array (user->frames_string, &user->frames,
                                         user->gparms->nout);

        fclaw_options_convert_double_array (user->density_string, &user->density,
                                            user->gparms->nout);
    }
    return FCLAW_NOEXIT;
}


static fclaw_exit_type_t
options_check_user (fclaw_app_t * app, void *package, void *registered)
{
    user_options_t* user = (user_options_t*) package;
    if (user->example < 1 || user->example > 2) {
        fclaw_global_essentialf ("Option --user:example must be 1 or 2\n");
        return FCLAW_EXIT_QUIET;
    }
    return FCLAW_NOEXIT;
}

static void
options_destroy_user (fclaw_app_t * a, void *package, void *registered)
{
    user_options_t* user = (user_options_t*) package;
    /* Destroy arrays used in options  */

    if (user->example == 2)
    {
        fclaw_options_destroy_array((void*) user->frames);
        fclaw_options_destroy_array((void*) user->density);
    }
}


static const fclaw_app_options_vtable_t options_vtable_user =
{
    options_register_user,
    options_postprocess_user,
    options_check_user,
    options_destroy_user
};

static
void register_user_options (fclaw_app_t * app,
                            const char *configfile,
                            user_options_t* user)
{
    FCLAW_ASSERT (app != NULL);

    fclaw_app_options_register (app,"user", configfile, &options_vtable_user,
                                user);
}


void run_program(fclaw_app_t* app, amr_options_t* gparms,
                 fc2d_clawpack46_options_t* clawpack_options,
                 user_options_t* user)
{
    sc_MPI_Comm            mpicomm;

    /* Local variables */
    double test_fpe;
    int i;

    mpicomm = fclaw_app_get_mpi_size_rank (app, NULL, NULL);

    switch (user->example)
    {
    case 1:
        fclaw_global_infof("Running example 1 ... Done\n");
        break;
    case 2:
        fclaw_global_infof("Running example 2 ... \n");
        for(i = 0; i < gparms->nout; i++)
        {
            fclaw_global_productionf("Density in frame %2d : %12.4f\n",
                                     user->frames[i],user->density[i]);
        }
        fclaw_debugf("Now let's trap a floating point error \n");
        test_fpe = sqrt(-1.0);
        fclaw_debugf("Done!\n");
        break;
    default:
        SC_ABORT_NOT_REACHED (); /* must be checked in torus_checkparms */
    }
}


int main (int argc, char **argv)
{
    fclaw_app_t *app;
    int first_arg;
    fclaw_exit_type_t vexit;

    /* Options */
    sc_options_t                  *options;
    amr_options_t                 samr_options, *gparms = &samr_options;
    fc2d_clawpack46_options_t     sclawpack_options, *clawpack_options = &sclawpack_options;
    user_options_t                suser_options, *user = &suser_options;

    int retval;

    /* Initialize application */
    app = fclaw_app_new (&argc, &argv, user);
    options = fclaw_app_get_options (app);

    fclaw_options_register_general (app, "fclaw_options.ini", gparms);
    fc2d_clawpack46_options_register(app,NULL,clawpack_options);

    /* User defined options (defined above) */
    user->gparms = gparms;
    register_user_options (app, "fclaw_options.ini", user);


    /* Read configuration file(s) */
    retval = fclaw_options_read_from_file(options);
    vexit =  fclaw_app_options_parse (app, &first_arg,"fclaw_options.ini.used");

    /* -------------------------------------------------------------
       - Run program
       ------------------------------------------------------------- */
    if (!retval & !vexit)
    {
        run_program(app, gparms, clawpack_options,user);
    }

    fclaw_app_destroy (app);

    return 0;
}
