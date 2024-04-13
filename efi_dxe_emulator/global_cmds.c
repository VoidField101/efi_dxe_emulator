/*
 * ______________________.___
 * \_   _____/\_   _____/|   |
 *  |    __)_  |    __)  |   |
 *  |        \ |     \   |   |
 * /_______  / \___  /   |___|
 *         \/      \/
 * ________  ____  ______________
 * \______ \ \   \/  /\_   _____/
 *  |    |  \ \     /  |    __)_
 *  |    `   \/     \  |        \
 *  /_______  /___/\  \/_______  /
 *          \/      \_/        \/
 * ___________             .__          __
 * \_   _____/ _____  __ __|  | _____ _/  |_  ___________
 *  |    __)_ /     \|  |  \  | \__  \\   __\/  _ \_  __ \
 *  |        \  Y Y  \  |  /  |__/ __ \|  | (  <_> )  | \/
 * /_______  /__|_|  /____/|____(____  /__|  \____/|__|
 *         \/      \/                \/
 *
 * EFI DXE Emulator
 *
 * An EFI DXE binary emulator based on Unicorn Engine
 *
 * Created by fG! on 01/05/16.
 * Copyright © 2016-2019 Pedro Vilaca. All rights reserved.
 * reverser@put.as - https://reverse.put.as
 *
 * global_cmds.c
 *
 * EFI Debugger global commands
 *
 * All advertising materials mentioning features or use of this software must display
 * the following acknowledgement: This product includes software developed by
 * Pedro Vilaca.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 
 * 1. Redistributions of source code must retain the above copyright notice, this list
 * of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice, this
 * list of conditions and the following disclaimer in the documentation and/or
 * other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software must
 * display the following acknowledgement: This product includes software developed
 * by Pedro Vilaca.
 * 4. Neither the name of the author nor the names of its contributors may be
 * used to endorse or promote products derived from this software without specific
 * prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "global_cmds.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <string.h>
#include <sys/queue.h>
#include <assert.h>
#include <unicorn/unicorn.h>

#include "pe_definitions.h"
#include "efi_definitions.h"
#include "logging.h"
#include "efi_runtime_hooks.h"
#include "efi_boot_hooks.h"
#include "config.h"
#include "nvram.h"
#include "debugger.h"
#include "cmds.h"
#include "loader.h"

extern struct bin_images_tailq g_images;

static int quit_cmd(const char *exp, uc_engine *uc);
static int run_cmd(const char *exp, uc_engine *uc);
static int continue_cmd(const char *exp, uc_engine *uc);
static int info_cmd(const char *exp, uc_engine *uc);

#pragma mark -
#pragma mark Functions to register the commands
#pragma mark -

void
register_global_cmds(uc_engine *uc)
{
    add_user_cmd("run", "r", run_cmd, "Start emulating target.\n\nrun", uc);
    add_user_cmd("quit", "q", quit_cmd, "Quit emulator.\n\nquit", uc);
    add_user_cmd("help", "h", help_cmd, "Help.\n\nhelp", uc);
    add_user_cmd("continue", "c", continue_cmd, "Continue running.\n\ncontinue", uc);
    add_user_cmd("info", NULL, info_cmd, "Info.\n\ninfo", uc);
    add_user_cmd("history", NULL, history_cmd, "Display comand line history.\n\nhistory", uc);
}

#pragma mark -
#pragma mark Commands functions
#pragma mark -

static int
quit_cmd(const char *exp, uc_engine *uc)
{
    uc_emu_stop(uc);
    return 1;
}

static int
run_cmd(const char *exp, uc_engine *uc)
{
    return 0;
}

static int
continue_cmd(const char *exp, uc_engine *uc)
{
    return 1;
}

static void
info_cmd_help(void)
{
    OUTPUT_MSG("\"info\" must be followed by the name of an info command.");
    OUTPUT_MSG("List of info subcommands:\n");
    OUTPUT_MSG("info target -- Information about main binary");
    OUTPUT_MSG("info all    -- Information about all mapped binaries");
    OUTPUT_MSG("");
}

static int
info_cmd(const char *exp, uc_engine *uc)
{
    char *local_exp = NULL;
    char *local_exp_ptr = NULL;
    local_exp_ptr = local_exp = strdup(exp);
    if (local_exp == NULL)
    {
        ERROR_MSG("strdup failed");
        return 0;
    }

    char *token = NULL;
    /* get rid of info string */
    strsep(&local_exp, " ");
    /* extract subcommand */
    token = strsep(&local_exp, " ");

    
    /* we need a target address */
    if (token == NULL)
    {
        info_cmd_help();
        free(local_exp_ptr);
        return 0;
    }
    
    struct bin_image *main_image = TAILQ_FIRST(&g_images);
    assert(main_image != NULL);
    
    if (strncmp(token, "target", 7) == 0)
    {
        OUTPUT_MSG("EFI Executable:\n%s", main_image->file_path);
        OUTPUT_MSG("Base address: 0%llx", main_image->base_addr);
        OUTPUT_MSG("Entrypoint: 0x%llx (0x%llx)", main_image->base_addr + main_image->entrypoint, main_image->entrypoint);
        OUTPUT_MSG("Image size: 0x%llx", main_image->buf_size);
        OUTPUT_MSG("Number of sections: %d", main_image->nr_sections);
    }
    else if (strncmp(token, "all", 3) == 0)
    {
        int count = 1;
        struct bin_image *tmp_image = NULL;
        TAILQ_FOREACH(tmp_image, &g_images, entries)
        {
            OUTPUT_MSG("---[ Image #%02d ]---", count++);
            OUTPUT_MSG("EFI Executable: \n%s", tmp_image->file_path);
            OUTPUT_MSG("Mapped address: 0x%llx", tmp_image->mapped_addr);
            OUTPUT_MSG("Mapped entrypoint: 0x%llx", tmp_image->mapped_addr + tmp_image->entrypoint);
            OUTPUT_MSG("Base address: 0%llx", tmp_image->base_addr);
            OUTPUT_MSG("Entrypoint: 0x%llx (0x%llx)", tmp_image->base_addr + tmp_image->entrypoint, tmp_image->entrypoint);
            OUTPUT_MSG("Image size: 0x%llx", tmp_image->buf_size);
            OUTPUT_MSG("Number of sections: %d", tmp_image->nr_sections);
        }
    }
    /* everything else is invalid */
    else
    {
        info_cmd_help();
        free(local_exp_ptr);
        return 0;
    }
    free(local_exp_ptr);
    return 0;
}
