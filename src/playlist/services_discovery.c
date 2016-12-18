/*****************************************************************************
 * services_discovery.c : Manage playlist services_discovery modules
 *****************************************************************************
 * Copyright (C) 1999-2004 VLC authors and VideoLAN
 * $Id$
 *
 * Authors: Clément Stenac <zorglub@videolan.org>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include <assert.h>

#include <vlc_common.h>
#include <vlc_playlist.h>
#include <vlc_services_discovery.h>
#include "playlist_internal.h"

struct vlc_sd_internal_t
{
    /* the playlist items for category and onelevel */
    playlist_item_t      *node;
    services_discovery_t *sd; /**< Loaded service discovery modules */
    char name[];
};

 /* A new item has been added to a certain sd */
static void playlist_sd_item_added(services_discovery_t *sd,
                                   input_item_t *p_input, const char *psz_cat)
{
    vlc_sd_internal_t *sds = sd->owner.sys;
    playlist_t *playlist = (playlist_t *)sd->obj.parent;
    playlist_item_t *parent;
    const char *longname = (sd->description != NULL) ? sd->description : "?";

    msg_Dbg(sd, "adding %s", p_input->psz_name ? p_input->psz_name : "(null)");

    playlist_Lock(playlist);
    if (sds->node == NULL)
        sds->node = playlist_NodeCreate(playlist, longname, playlist->p_root,
                                        PLAYLIST_END,
                                        PLAYLIST_RO_FLAG|PLAYLIST_SKIP_FLAG);

    /* If p_parent is in root category (this is clearly a hack) and we have a cat */
    if (psz_cat == NULL)
        parent = sds->node;
    else
    {
        parent = playlist_ChildSearchName(sds->node, psz_cat);
        if (parent == NULL)
            parent = playlist_NodeCreate(playlist, psz_cat, sds->node,
                                         PLAYLIST_END,
                                         PLAYLIST_RO_FLAG|PLAYLIST_SKIP_FLAG);
    }

    playlist_NodeAddInput(playlist, p_input, parent, 0, PLAYLIST_END);
    playlist_Unlock(playlist);
}

 /* A new item has been removed from a certain sd */
static void playlist_sd_item_removed(services_discovery_t *sd,
                                     input_item_t *p_input)
{
    vlc_sd_internal_t *sds = sd->owner.sys;
    playlist_t *p_playlist = (playlist_t *)sd->obj.parent;
    playlist_item_t *p_sd_node = sds->node;

    PL_LOCK;
    playlist_item_t *p_item =
        playlist_ItemFindFromInputAndRoot( p_playlist, p_input, p_sd_node );
    if( !p_item )
    {
        PL_UNLOCK; return;
    }
    playlist_item_t *p_parent = p_item->p_parent;
    /* if the item was added under a category and the category node
       becomes empty, delete that node as well */
    if( p_parent->i_children > 1 || p_parent == p_sd_node )
        playlist_NodeDelete( p_playlist, p_item, false );
    else
        playlist_NodeDelete( p_playlist, p_parent, true );
    PL_UNLOCK;
}

int playlist_ServicesDiscoveryAdd(playlist_t *playlist, const char *chain)
{
    vlc_sd_internal_t *sds = malloc(sizeof (*sds) + strlen(chain) + 1);
    if (unlikely(sds == NULL))
        return VLC_ENOMEM;

    sds->node = NULL;

    struct services_discovery_owner_t owner = {
        sds,
        playlist_sd_item_added,
        playlist_sd_item_removed,
    };

    /* Perform the addition */
    sds->sd = vlc_sd_Create(VLC_OBJECT(playlist), chain, &owner);
    if (unlikely(sds->sd == NULL))
    {
        free(sds);
        return VLC_ENOMEM;
    }

    strcpy(sds->name, chain);

    playlist_Lock(playlist);
    /* Backward compatibility with Qt UI: create the node even if the SD
     * has not discovered any item. */
    if (sds->node == NULL && sds->sd->description != NULL)
        sds->node = playlist_NodeCreate(playlist, sds->sd->description,
                                        playlist->p_root, PLAYLIST_END,
                                        PLAYLIST_RO_FLAG|PLAYLIST_SKIP_FLAG);

    TAB_APPEND(pl_priv(playlist)->i_sds, pl_priv(playlist)->pp_sds, sds);
    playlist_Unlock(playlist);
    return VLC_SUCCESS;
}

static void playlist_ServicesDiscoveryInternalRemove(playlist_t *playlist,
                                                     vlc_sd_internal_t *sds)
{
    assert(sds->sd != NULL);
    vlc_sd_Destroy(sds->sd);

    /* Remove the sd playlist node if it exists */
    playlist_Lock(playlist);
    if (sds->node != NULL)
        playlist_NodeDelete(playlist, sds->node, true);
    playlist_Unlock(playlist);

    free(sds);
}


int playlist_ServicesDiscoveryRemove(playlist_t *playlist, const char *name)
{
    playlist_private_t *priv = pl_priv(playlist);
    vlc_sd_internal_t *sds = NULL;

    playlist_Lock(playlist);
    for (int i = 0; i < priv->i_sds; i++)
    {
        vlc_sd_internal_t *entry = priv->pp_sds[i];

        if (!strcmp(name, entry->name))
        {
            REMOVE_ELEM(priv->pp_sds, priv->i_sds, i);
            sds = entry;
            break;
        }
    }
    playlist_Unlock(playlist);

    if (sds == NULL)
    {
        msg_Warn(playlist, "discovery %s is not loaded", name);
        return VLC_EGENERIC;
    }

    playlist_ServicesDiscoveryInternalRemove(playlist, sds);
    return VLC_SUCCESS;
}

bool playlist_IsServicesDiscoveryLoaded( playlist_t * p_playlist,
                                         const char *psz_name )
{
    playlist_private_t *priv = pl_priv( p_playlist );
    bool found = false;
    PL_LOCK;

    for( int i = 0; i < priv->i_sds; i++ )
    {
        vlc_sd_internal_t *sds = priv->pp_sds[i];

        if (!strcmp(psz_name, sds->name))
        {
            found = true;
            break;
        }
    }
    PL_UNLOCK;
    return found;
}

int playlist_ServicesDiscoveryControl( playlist_t *p_playlist, const char *psz_name, int i_control, ... )
{
    playlist_private_t *priv = pl_priv( p_playlist );
    int i_ret = VLC_EGENERIC;
    int i;

    PL_LOCK;
    for( i = 0; i < priv->i_sds; i++ )
    {
        vlc_sd_internal_t *sds = priv->pp_sds[i];
        if (!strcmp(psz_name, sds->name))
        {
            va_list args;
            va_start( args, i_control );
            i_ret = vlc_sd_control(sds->sd, i_control, args );
            va_end( args );
            break;
        }
    }

    assert( i != priv->i_sds );
    PL_UNLOCK;

    return i_ret;
}

void playlist_ServicesDiscoveryKillAll(playlist_t *playlist)
{
    playlist_private_t *priv = pl_priv(playlist);

    for (int i = 0; i < priv->i_sds; i++)
        playlist_ServicesDiscoveryInternalRemove(playlist, priv->pp_sds[i]);

    TAB_CLEAN(priv->i_sds, priv->pp_sds);
}
