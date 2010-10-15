/*
	Onion HTTP server library
	Copyright (C) 2010 David Moreno Montero

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU Affero General Public License as
	published by the Free Software Foundation, either version 3 of the
	License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Affero General Public License for more details.

	You should have received a copy of the GNU Affero General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
	*/

#include <string.h>
#include <malloc.h>
#include <unistd.h>
#include <regex.h>

#include <onion_response.h>

#include "onion_handler_static.h"

struct onion_handler_path_data_t{
	regex_t path;
	onion_handler *inside;
};

typedef struct onion_handler_path_data_t onion_handler_path_data;

int onion_handler_path_handler(onion_handler *handler, onion_request *request){
	onion_handler_path_data *d=handler->priv_data;
	regmatch_t match[1];
	
	if (regexec(&d->path, request->path, 1, match, 0)!=0)
		return 0;
	
	request->path=&request->path[match[0].rm_eo];
	
	return onion_handler_handle(d->inside, request);
}


void onion_handler_path_delete(void *data){
	onion_handler_path_data *d=data;
	regfree(&d->path);
	onion_handler_free(d->inside);
}

/**
 * @short Creates an path handler. If the path matches the regex, it reomves that from the regexp and goes to the inside_level.
 *
 * If on the inside level nobody answers, it just returns NULL, so ->next can answer.
 */
onion_handler *onion_handler_path(const char *path, onion_handler *inside_level){
	onion_handler *ret;
	ret=malloc(sizeof(onion_handler));
	memset(ret,0,sizeof(onion_handler));
	
	onion_handler_path_data *priv_data=malloc(sizeof(onion_handler_path_data));

	priv_data->inside=inside_level;
	
	// Path is a little bit more complicated, its an regexp.
	int err;
	if (path)
		err=regcomp(&priv_data->path, path, REG_EXTENDED);
	else
		err=regcomp(&priv_data->path, "", REG_EXTENDED); // empty regexp, always true. should be fast enough. 
	if (err){
		char buffer[1024];
		regerror(err, &priv_data->path, buffer, sizeof(buffer));
		fprintf(stderr, "%s:%d Error analyzing regular expression '%s': %s.\n", __FILE__,__LINE__, path, buffer);
		onion_handler_path_delete(priv_data);
		return NULL;
	}
	
	ret->handler=onion_handler_path_handler;
	ret->priv_data=priv_data;
	ret->priv_data_delete=onion_handler_path_delete;
	
	return ret;
}
