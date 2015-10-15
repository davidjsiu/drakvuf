/*********************IMPORTANT DRAKVUF LICENSE TERMS***********************
 *                                                                         *
 * DRAKVUF Dynamic Malware Analysis System (C) 2014 Tamas K Lengyel.       *
 * Tamas K Lengyel is hereinafter referred to as the author.               *
 * This program is free software; you may redistribute and/or modify it    *
 * under the terms of the GNU General Public License as published by the   *
 * Free Software Foundation; Version 2 ("GPL"), BUT ONLY WITH ALL OF THE   *
 * CLARIFICATIONS AND EXCEPTIONS DESCRIBED HEREIN.  This guarantees your   *
 * right to use, modify, and redistribute this software under certain      *
 * conditions.  If you wish to embed DRAKVUF technology into proprietary   *
 * software, alternative licenses can be aquired from the author.          *
 *                                                                         *
 * Note that the GPL places important restrictions on "derivative works",  *
 * yet it does not provide a detailed definition of that term.  To avoid   *
 * misunderstandings, we interpret that term as broadly as copyright law   *
 * allows.  For example, we consider an application to constitute a        *
 * derivative work for the purpose of this license if it does any of the   *
 * following with any software or content covered by this license          *
 * ("Covered Software"):                                                   *
 *                                                                         *
 * o Integrates source code from Covered Software.                         *
 *                                                                         *
 * o Reads or includes copyrighted data files.                             *
 *                                                                         *
 * o Is designed specifically to execute Covered Software and parse the    *
 * results (as opposed to typical shell or execution-menu apps, which will *
 * execute anything you tell them to).                                     *
 *                                                                         *
 * o Includes Covered Software in a proprietary executable installer.  The *
 * installers produced by InstallShield are an example of this.  Including *
 * DRAKVUF with other software in compressed or archival form does not     *
 * trigger this provision, provided appropriate open source decompression  *
 * or de-archiving software is widely available for no charge.  For the    *
 * purposes of this license, an installer is considered to include Covered *
 * Software even if it actually retrieves a copy of Covered Software from  *
 * another source during runtime (such as by downloading it from the       *
 * Internet).                                                              *
 *                                                                         *
 * o Links (statically or dynamically) to a library which does any of the  *
 * above.                                                                  *
 *                                                                         *
 * o Executes a helper program, module, or script to do any of the above.  *
 *                                                                         *
 * This list is not exclusive, but is meant to clarify our interpretation  *
 * of derived works with some common examples.  Other people may interpret *
 * the plain GPL differently, so we consider this a special exception to   *
 * the GPL that we apply to Covered Software.  Works which meet any of     *
 * these conditions must conform to all of the terms of this license,      *
 * particularly including the GPL Section 3 requirements of providing      *
 * source code and allowing free redistribution of the work as a whole.    *
 *                                                                         *
 * Any redistribution of Covered Software, including any derived works,    *
 * must obey and carry forward all of the terms of this license, including *
 * obeying all GPL rules and restrictions.  For example, source code of    *
 * the whole work must be provided and free redistribution must be         *
 * allowed.  All GPL references to "this License", are to be treated as    *
 * including the terms and conditions of this license text as well.        *
 *                                                                         *
 * Because this license imposes special exceptions to the GPL, Covered     *
 * Work may not be combined (even as part of a larger work) with plain GPL *
 * software.  The terms, conditions, and exceptions of this license must   *
 * be included as well.  This license is incompatible with some other open *
 * source licenses as well.  In some cases we can relicense portions of    *
 * DRAKVUF or grant special permissions to use it in other open source     *
 * software.  Please contact tamas.k.lengyel@gmail.com with any such       *
 * requests.  Similarly, we don't incorporate incompatible open source     *
 * software into Covered Software without special permission from the      *
 * copyright holders.                                                      *
 *                                                                         *
 * If you have any questions about the licensing restrictions on using     *
 * DRAKVUF in other works, are happy to help.  As mentioned above,         *
 * alternative license can be requested from the author to integrate       *
 * DRAKVUF into proprietary applications and appliances.  Please email     *
 * tamas.k.lengyel@gmail.com for further information.                      *
 *                                                                         *
 * If you have received a written license agreement or contract for        *
 * Covered Software stating terms other than these, you may choose to use  *
 * and redistribute Covered Software under those terms instead of these.   *
 *                                                                         *
 * Source is provided to this software because we believe users have a     *
 * right to know exactly what a program is going to do before they run it. *
 * This also allows you to audit the software for security holes.          *
 *                                                                         *
 * Source code also allows you to port DRAKVUF to new platforms, fix bugs, *
 * and add new features.  You are highly encouraged to submit your changes *
 * on https://github.com/tklengyel/drakvuf, or by other methods.           *
 * By sending these changes, it is understood (unless you specify          *
 * otherwise) that you are offering unlimited, non-exclusive right to      *
 * reuse, modify, and relicense the code.  DRAKVUF will always be          *
 * available Open Source, but this is important because the inability to   *
 * relicense code has caused devastating problems for other Free Software  *
 * projects (such as KDE and NASM).                                        *
 * To specify special license conditions of your contributions, just say   *
 * so when you send them.                                                  *
 *                                                                         *
 * This program is distributed in the hope that it will be useful, but     *
 * WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the DRAKVUF   *
 * license file for more details (it's in a COPYING file included with     *
 * DRAKVUF, and also available from                                        *
 * https://github.com/tklengyel/drakvuf/COPYING)                           *
 *                                                                         *
 ***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include "structures.h"
#include "injector.h"
#include "vmi.h"
#include "pooltag.h"
#include "xen_helper.h"
#include "win-symbols.h"

static drakvuf_t drakvuf;

static void close_handler(int sig) {
    drakvuf.interrupted = sig;
}

static inline void free_sym_config(struct sym_config *sym_config) {
    uint32_t i;
    if (!sym_config) return;

    for (i=0; i < sym_config->sym_count; i++) {
        free(sym_config->syms[i].name);
    }
    free(sym_config->syms);
    free(sym_config);
}

static inline void drakvuf_init(drakvuf_t *drakvuf) {
    memset(drakvuf, 0, sizeof(drakvuf_t));
    drakvuf->pooltags = pooltag_build_tree();
    xen_init_interface(&drakvuf->xen);
}

static inline void close_drakvuf(drakvuf_t *drakvuf) {
    free_sym_config(drakvuf->sym_config);
    g_tree_destroy(drakvuf->pooltags);
    xen_free_interface(drakvuf->xen);
}

int main(int argc, char** argv) {

    int c;
    char *executable = NULL;
    char *domain = NULL;
    vmi_pid_t injection_pid = -1;
    struct sigaction act;

    printf("%s v%s\n", PACKAGE_NAME, PACKAGE_VERSION);

    if (argc < 4) {
        printf("Required input:\n"
               "\t -r <rekall profile>\n"
               "\t -d <domain ID or name>\n"
               "Optional inputs:\n"
               "\t -i <injection pid>\n"
               "\t -e <injection executable_path>\n"
               "\t -t <timeout in seconds>\n");
        return 1;
    }

    drakvuf_init(&drakvuf);

    while ((c = getopt (argc, argv, "r:d:p:e:t:")) != -1)
    switch (c)
    {
    case 'r':
        drakvuf.rekall_profile = optarg;
        break;
    case 'd':
        domain = optarg;
        break;
    case 'i':
        injection_pid = atoi(optarg);
        break;
    case 'e':
        executable = optarg;
    case 't':
        drakvuf.timeout = atoi(optarg);
    default:
        printf("Unrecognized option: %c\n", c);
        goto exit;
    }

    drakvuf.sym_config = get_all_symbols(drakvuf.rekall_profile);
    if (!drakvuf.sym_config)
    {
        printf("Failed to parse Rekall profile at %s\n", drakvuf.rekall_profile);
        goto exit;
    }

    get_dom_info(drakvuf.xen, domain, &drakvuf.domID, &drakvuf.dom_name);

    init_vmi(&drakvuf);

    if (!drakvuf.vmi) {
        goto exit;
    }

    if (injection_pid > 0 && executable) {
        int rc = start_app(&drakvuf, injection_pid, executable);

        if (!rc) {
            printf("Process startup failed\n");
            goto exit;
        }
    }

    inject_traps(&drakvuf);

    /* for a clean exit */
    act.sa_handler = close_handler;
    act.sa_flags = 0;
    sigemptyset(&act.sa_mask);
    sigaction(SIGHUP, &act, NULL);
    sigaction(SIGTERM, &act, NULL);
    sigaction(SIGINT, &act, NULL);
    sigaction(SIGALRM, &act, NULL);

    drakvuf_loop(&drakvuf);

    close_vmi(&drakvuf);

exit:
    close_drakvuf(&drakvuf);

    return 0;
}
