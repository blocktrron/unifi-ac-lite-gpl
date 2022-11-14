/*
 * Copyright (C) 2013 Felix Fietkau <nbd@openwrt.org>
 * Copyright (C) 2013 John Crispin <blogic@openwrt.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 2.1
 * as published by the Free Software Foundation
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#define _GNU_SOURCE
#include <sys/syscall.h>
#include <sys/mman.h>
#include <sys/utsname.h>

#include <stdlib.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <values.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <libgen.h>
#include <glob.h>
#include <elf.h>

#include <libubox/avl.h>
#include <libubox/avl-cmp.h>
#include <libubox/utils.h>
#include <libubox/ulog.h>

#define DEF_MOD_PATH "/modules/%s/"

enum {
	SCANNED,
	PROBE,
	LOADED,
};

struct module {
	char *name;
	char *depends;
	char *opts;

	int size;
	int usage;
	int state;
	int error;
	int refcnt;			/* number of references from module_node.m */
};

struct module_node {
	struct avl_node avl;
	struct module *m;
	bool is_alias;
};

static struct avl_tree modules;

static char **module_folders = NULL;

static void free_module(struct module *m);

static int init_module_folders(void)
{
	int n = 0;
	struct stat st;
	struct utsname ver;
	char *s, *e, *p, path[256], ldpath[256];

	e = ldpath;
	s = getenv("LD_LIBRARY_PATH");

	if (s)
		e += snprintf(ldpath, sizeof(ldpath), "%s:", s);

	e += snprintf(e, sizeof(ldpath) - (e - ldpath), "/lib");

	uname(&ver);

	for (s = p = ldpath; p <= e; p++) {
		if (*p != ':' && *p != '\0')
			continue;

		*p = 0;
		snprintf(path, sizeof(path), "%s" DEF_MOD_PATH, s, ver.release);

		if (!stat(path, &st) && S_ISDIR(st.st_mode)) {
			module_folders = realloc(module_folders, sizeof(p) * (n + 2));

			if (!module_folders) {
				ULOG_ERR("out of memory\n");
				return -1;
			}

			module_folders[n++] = strdup(path);
		}

		s = p + 1;
	}

	if (!module_folders)
		return -1;

	module_folders[n] = NULL;
	return 0;
}

static struct module *find_module(const char *name)
{
	struct module_node *mn;
	mn = avl_find_element(&modules, name, mn, avl);
	if (mn)
		return mn->m;
	else
		return NULL;
}

static void free_modules(void)
{
	struct module_node *mn, *tmp;

	avl_remove_all_elements(&modules, mn, avl, tmp) {
		struct module *m = mn->m;

		m->refcnt -= 1;
		if (m->refcnt == 0)
			free_module(m);
		free(mn);
	}
}

static char* get_module_path(char *name)
{
	char **p;
	static char path[256];
	struct stat s;

	if (!stat(name, &s) && S_ISREG(s.st_mode))
		return name;

	for (p = module_folders; *p; p++) {
		snprintf(path, sizeof(path), "%s%s.ko", *p, name);
		if (!stat(path, &s) && S_ISREG(s.st_mode))
			return path;
	}

	return NULL;
}

static char* get_module_name(char *path)
{
	static char name[33];
	char *t;

	strncpy(name, basename(path), sizeof(name) - 1);

	t = strstr(name, ".ko");
	if (t)
		*t = '\0';

	return name;
}

static int elf64_find_section(char *map, const char *section, unsigned int *offset, unsigned int *size)
{
	const char *secnames;
	Elf64_Ehdr *e;
	Elf64_Shdr *sh;
	int i;

	e = (Elf64_Ehdr *) map;
	sh = (Elf64_Shdr *) (map + e->e_shoff);

	secnames = map + sh[e->e_shstrndx].sh_offset;
	for (i = 0; i < e->e_shnum; i++) {
		if (!strcmp(section, secnames + sh[i].sh_name)) {
			*size = sh[i].sh_size;
			*offset = sh[i].sh_offset;
			return 0;
		}
	}

	return -1;
}

static int elf32_find_section(char *map, const char *section, unsigned int *offset, unsigned int *size)
{
	const char *secnames;
	Elf32_Ehdr *e;
	Elf32_Shdr *sh;
	int i;

	e = (Elf32_Ehdr *) map;
	sh = (Elf32_Shdr *) (map + e->e_shoff);

	secnames = map + sh[e->e_shstrndx].sh_offset;
	for (i = 0; i < e->e_shnum; i++) {
		if (!strcmp(section, secnames + sh[i].sh_name)) {
			*size = sh[i].sh_size;
			*offset = sh[i].sh_offset;
			return 0;
		}
	}

	return -1;
}

static int elf_find_section(char *map, const char *section, unsigned int *offset, unsigned int *size)
{
	int clazz = map[EI_CLASS];

	if (clazz == ELFCLASS32)
		return elf32_find_section(map, section, offset, size);
	else if (clazz == ELFCLASS64)
		return elf64_find_section(map, section, offset, size);

	ULOG_ERR("unknown elf format %d\n", clazz);

	return -1;
}

static struct module_node *
alloc_module_node(const char *name, struct module *m, bool is_alias)
{
	struct module_node *mn;
	char *_name;

	mn = calloc_a(sizeof(*mn),
		&_name, strlen(name) + 1);
	if (mn) {
		mn->avl.key = strcpy(_name, name);
		mn->m = m;
		mn->is_alias = is_alias;
		avl_insert(&modules, &mn->avl);
		m->refcnt += 1;
	}
	return mn;
}

static struct module *
alloc_module(const char *name, const char * const *aliases, int naliases, const char *depends, int size)
{
	struct module *m;
	char *_name, *_dep;
	int i;

	m = calloc_a(sizeof(*m),
		&_name, strlen(name) + 1,
		&_dep, depends ? strlen(depends) + 2 : 0);
	if (!m)
		return NULL;

	m->name = strcpy(_name, name);
	m->opts = 0;

	if (depends) {
		m->depends = strcpy(_dep, depends);
		while (*_dep) {
			if (*_dep == ',')
				*_dep = '\0';
			_dep++;
		}
	}
	m->size = size;

	m->refcnt = 0;
	alloc_module_node(m->name, m, false);
	for (i = 0; i < naliases; i++)
		alloc_module_node(aliases[i], m, true);

	return m;
}

static void free_module(struct module *m)
{
	if (m->opts)
		free(m->opts);
	free(m);
}

static int scan_loaded_modules(void)
{
	size_t buf_len = 0;
	char *buf = NULL;
	FILE *fp;

	fp = fopen("/proc/modules", "r");
	if (!fp) {
		ULOG_ERR("failed to open /proc/modules\n");
		return -1;
	}

	while (getline(&buf, &buf_len, fp) > 0) {
		struct module m;
		struct module *n;

		m.name = strtok(buf, " ");
		m.size = atoi(strtok(NULL, " "));
		m.usage = atoi(strtok(NULL, " "));
		m.depends = strtok(NULL, " ");

		if (!m.name || !m.depends)
			continue;

		n = find_module(m.name);
		if (!n) {
			/* possibly a module outside /lib/modules/<ver>/ */
			n = alloc_module(m.name, NULL, 0, m.depends, m.size);
		}
		n->usage = m.usage;
		n->state = LOADED;
	}
	free(buf);
	fclose(fp);

	return 0;
}

static struct module* get_module_info(const char *module, const char *name)
{
	int fd = open(module, O_RDONLY);
	unsigned int offset, size;
	char *map = MAP_FAILED, *strings, *dep = NULL;
	const char *aliases[32] = { 0 };
	int naliases = 0;
	struct module *m = NULL;
	struct stat s;

	if (fd < 0) {
		ULOG_ERR("failed to open %s\n", module);
		goto out;
	}

	if (fstat(fd, &s) == -1) {
		ULOG_ERR("failed to stat %s\n", module);
		goto out;
	}

	map = mmap(NULL, s.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	if (map == MAP_FAILED) {
		ULOG_ERR("failed to mmap %s\n", module);
		goto out;
	}

	if (elf_find_section(map, ".modinfo", &offset, &size)) {
		ULOG_ERR("failed to load the .modinfo section from %s\n", module);
		goto out;
	}

	strings = map + offset;
	while (true) {
		char *sep;
		int len;

		while (!strings[0])
			strings++;
		if (strings >= map + offset + size)
			break;
		sep = strstr(strings, "=");
		if (!sep)
			break;
		len = sep - strings;
		sep++;
		if (!strncmp(strings, "depends=", len + 1))
			dep = sep;
		else if (!strncmp(strings, "alias=", len + 1)) {
			if (naliases < ARRAY_SIZE(aliases))
				aliases[naliases++] = sep;
			else
				ULOG_WARN("module %s has more than %d aliases: truncated",
						name, ARRAY_SIZE(aliases));
		}
		strings = &sep[strlen(sep)];
	}

	m = alloc_module(name, aliases, naliases, dep, s.st_size);

	if (m)
		m->state = SCANNED;

out:
	if (map != MAP_FAILED)
		munmap(map, s.st_size);

	if (fd >= 0)
		close(fd);

	return m;
}

static int scan_module_folder(const char *dir)
{
	int gl_flags = GLOB_NOESCAPE | GLOB_MARK;
	struct utsname ver;
	char *path;
	glob_t gl;
	int j;

	uname(&ver);
	path = alloca(strlen(dir) + sizeof("*.ko") + 1);
	sprintf(path, "%s*.ko", dir);

	if (glob(path, gl_flags, NULL, &gl) < 0)
		return -1;

	for (j = 0; j < gl.gl_pathc; j++) {
		char *name = get_module_name(gl.gl_pathv[j]);
		struct module *m;

		if (!name)
			continue;

		m = find_module(name);
		if (!m)
			get_module_info(gl.gl_pathv[j], name);
	}

	globfree(&gl);

	return 0;
}

static int scan_module_folders(void)
{
	int rv = 0;
	char **p;

	if (init_module_folders())
		return -1;

	for (p = module_folders; *p; p++)
		rv |= scan_module_folder(*p);

	return rv;
}

static int print_modinfo(char *module)
{
	int fd = open(module, O_RDONLY);
	unsigned int offset, size;
	struct stat s;
	char *map = MAP_FAILED, *strings;
	int rv = -1;

	if (fd < 0) {
		ULOG_ERR("failed to open %s\n", module);
		goto out;
	}

	if (fstat(fd, &s) == -1) {
		ULOG_ERR("failed to stat %s\n", module);
		goto out;
	}

	map = mmap(NULL, s.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	if (map == MAP_FAILED) {
		ULOG_ERR("failed to mmap %s\n", module);
		goto out;
	}

	if (elf_find_section(map, ".modinfo", &offset, &size)) {
		ULOG_ERR("failed to load the .modinfo section from %s\n", module);
		goto out;
	}

	strings = map + offset;
	printf("module:\t\t%s\n", module);
	while (true) {
		char *dup = NULL;
		char *sep;

		while (!strings[0])
			strings++;
		if (strings >= map + offset + size)
			break;
		sep = strstr(strings, "=");
		if (!sep)
			break;
		dup = strndup(strings, sep - strings);
		sep++;
		if (strncmp(strings, "parm", 4)) {
			if (strlen(dup) < 7)
				printf("%s:\t\t%s\n",  dup, sep);
			else
				printf("%s:\t%s\n",  dup, sep);
		}
		strings = &sep[strlen(sep)];
		if (dup)
			free(dup);
	}

	rv = 0;

out:
	if (map != MAP_FAILED)
		munmap(map, s.st_size);

	if (fd >= 0)
		close(fd);

	return rv;
}

static int deps_available(struct module *m, int verbose)
{
	char *dep;
	int err = 0;

	if (!m->depends || !strcmp(m->depends, "-") || !strcmp(m->depends, ""))
		return 0;

	dep = m->depends;

	while (*dep) {
		m = find_module(dep);

		if (verbose && !m)
			ULOG_ERR("missing dependency %s\n", dep);
		if (verbose && m && (m->state != LOADED))
			ULOG_ERR("dependency not loaded %s\n", dep);
		if (!m || (m->state != LOADED))
			err++;
		dep += strlen(dep) + 1;
	}

	return err;
}

static int insert_module(char *path, const char *options)
{
	void *data = 0;
	struct stat s;
	int fd, ret = -1;

	if (stat(path, &s)) {
		ULOG_ERR("missing module %s\n", path);
		return ret;
	}

	fd = open(path, O_RDONLY);
	if (fd < 0) {
		ULOG_ERR("cannot open %s\n", path);
		return ret;
	}

	data = malloc(s.st_size);
	if (!data) {
		ULOG_ERR("out of memory\n");
		goto out;
	}

	if (read(fd, data, s.st_size) == s.st_size) {
		ret = syscall(__NR_init_module, data, (unsigned long) s.st_size, options);
		if (errno == EEXIST)
			ret = 0;
	}
	else
		ULOG_ERR("failed to read full module %s\n", path);

out:
	close(fd);
	free(data);

	return ret;
}

static void load_moddeps(struct module *_m)
{
	char *dep;
	struct module *m;

	if (!strcmp(_m->depends, "-") || !strcmp(_m->depends, ""))
		return;

	dep = _m->depends;

	while (*dep) {
		m = find_module(dep);

		if (!m)
			ULOG_ERR("failed to find dependency %s\n", dep);
		if (m && (m->state != LOADED)) {
			m->state = PROBE;
			load_moddeps(m);
		}

		dep = dep + strlen(dep) + 1;
	}
}

static int iterations = 0;
static int load_modprobe(void)
{
	int loaded, todo;
	struct module_node *mn;
	struct module *m;

	avl_for_each_element(&modules, mn, avl) {
		if (mn->is_alias)
			continue;
		m = mn->m;
		if (m->state == PROBE)
			load_moddeps(m);
	}

	do {
		loaded = 0;
		todo = 0;
		avl_for_each_element(&modules, mn, avl) {
			if (mn->is_alias)
				continue;
			m = mn->m;
			if ((m->state == PROBE) && (!deps_available(m, 0)) && m->error < 2) {
				if (!insert_module(get_module_path(m->name), (m->opts) ? (m->opts) : (""))) {
					m->state = LOADED;
					m->error = 0;
					loaded++;
					continue;
				}

				if (++m->error > 1)
					ULOG_ERR("failed to load %s\n", m->name);
			}

			if ((m->state == PROBE) || m->error)
				todo++;
		}
		iterations++;
	} while (loaded);

	return todo;
}

static int print_insmod_usage(void)
{
	ULOG_INFO("Usage:\n\tinsmod filename [args]\n");

	return -1;
}

static int print_modprobe_usage(void)
{
	ULOG_INFO("Usage:\n\tmodprobe [-q] filename\n");

	return -1;
}

static int print_usage(char *arg)
{
	ULOG_INFO("Usage:\n\t%s module\n", arg);

	return -1;
}

static int main_insmod(int argc, char **argv)
{
	char *name, *cur, *options;
	int i, ret, len;

	if (argc < 2)
		return print_insmod_usage();

	name = get_module_name(argv[1]);
	if (!name) {
		ULOG_ERR("cannot find module - %s\n", argv[1]);
		return -1;
	}

	if (scan_loaded_modules())
		return -1;

	if (find_module(name)) {
		ULOG_ERR("module is already loaded - %s\n", name);
		return -1;

	}

	free_modules();

	for (len = 0, i = 2; i < argc; i++)
		len += strlen(argv[i]) + 1;

	options = malloc(len);
	if (!options) {
		ULOG_ERR("out of memory\n");
		ret = -1;
		goto err;
	}

	options[0] = 0;
	cur = options;
	for (i = 2; i < argc; i++) {
		if (options[0]) {
			*cur = ' ';
			cur++;
		}
		cur += sprintf(cur, "%s", argv[i]);
	}

	if (init_module_folders()) {
		fprintf(stderr, "Failed to find the folder holding the modules\n");
		ret = -1;
		goto err;
	}

	if (get_module_path(argv[1])) {
		name = argv[1];
	} else if (!get_module_path(name)) {
		fprintf(stderr, "Failed to find %s. Maybe it is a built in module ?\n", name);
		ret = -1;
		goto err;
	}

	ret = insert_module(get_module_path(name), options);

	if (ret)
		ULOG_ERR("failed to insert %s\n", get_module_path(name));

err:
	free(options);
	return ret;
}

static int main_rmmod(int argc, char **argv)
{
	struct module *m;
	char *name;
	int ret;

	if (argc != 2)
		return print_usage("rmmod");

	if (scan_loaded_modules())
		return -1;

	name = get_module_name(argv[1]);
	m = find_module(name);
	if (!m) {
		ULOG_ERR("module is not loaded\n");
		return -1;
	}
	ret = syscall(__NR_delete_module, m->name, 0);

	if (ret)
		ULOG_ERR("unloading the module failed\n");

	free_modules();

	return ret;
}

static int main_lsmod(int argc, char **argv)
{
	struct module_node *mn;
	struct module *m;
	char *dep;

	if (scan_loaded_modules())
		return -1;

	avl_for_each_element(&modules, mn, avl) {
		if (mn->is_alias)
			continue;
		m = mn->m;
		if (m->state == LOADED) {
			printf("%-20s%8d%3d ",
				m->name, m->size, m->usage);
			if (m->depends && strcmp(m->depends, "-") && strcmp(m->depends, "")) {
				dep = m->depends;
				while (*dep) {
					printf("%s", dep);
					dep = dep + strlen(dep) + 1;
					if (*dep)
						printf(",");
				}
			}
			printf("\n");
		}
	}

	free_modules();

	return 0;
}

static int main_modinfo(int argc, char **argv)
{
	struct module *m;
	char *name;

	if (argc != 2)
		return print_usage("modinfo");

	if (scan_module_folders())
		return -1;

	name = get_module_name(argv[1]);
	m = find_module(name);
	if (!m) {
		ULOG_ERR("cannot find module - %s\n", argv[1]);
		return -1;
	}

	name = get_module_path(m->name);
	if (!name) {
		ULOG_ERR("cannot find path of module - %s\n", m->name);
		return -1;
	}

	print_modinfo(name);

	return 0;
}

static int main_modprobe(int argc, char **argv)
{
	struct module_node *mn;
	struct module *m;
	char *name;
	char *mod = NULL;
	int opt;
	bool quiet = false;

	while ((opt = getopt(argc, argv, "q")) != -1 ) {
		switch (opt) {
			case 'q': /* shhhh! */
				quiet = true;
				break;
			default: /* '?' */
				return print_modprobe_usage();
				break;
			}
	}

	if (optind >= argc)
		return print_modprobe_usage(); /* expected module after options */

	mod = argv[optind];

	if (scan_module_folders())
		return -1;

	if (scan_loaded_modules())
		return -1;

	name = get_module_name(mod);
	m = find_module(name);
	if (m && m->state == LOADED) {
		if (!quiet)
			ULOG_ERR("%s is already loaded\n", name);
		return 0;
	} else if (!m) {
		if (!quiet)
			ULOG_ERR("failed to find a module named %s\n", name);
		return -1;
	} else {
		int fail;

		m->state = PROBE;

		fail = load_modprobe();

		if (fail) {
			ULOG_ERR("%d module%s could not be probed\n",
			         fail, (fail == 1) ? ("") : ("s"));

			avl_for_each_element(&modules, mn, avl) {
				if (mn->is_alias)
					continue;
				m = mn->m;
				if ((m->state == PROBE) || m->error)
					ULOG_ERR("- %s\n", m->name);
			}
		}
	}

	free_modules();

	return 0;
}

static int main_loader(int argc, char **argv)
{
	int gl_flags = GLOB_NOESCAPE | GLOB_MARK;
	char *dir = "/etc/modules.d/";
	struct module_node *mn;
	struct module *m;
	glob_t gl;
	char *path;
	int fail, j;

	if (argc > 1)
		dir = argv[1];

	path = malloc(strlen(dir) + 2);
	if (!path) {
		ULOG_ERR("out of memory\n");
		return -1;
	}

	strcpy(path, dir);
	strcat(path, "*");

	if (scan_module_folders()) {
		free (path);
		return -1;
	}

	if (scan_loaded_modules()) {
		free (path);
		return -1;
	}

	ULOG_INFO("loading kernel modules from %s\n", path);

	if (glob(path, gl_flags, NULL, &gl) < 0)
		goto out;

	for (j = 0; j < gl.gl_pathc; j++) {
		FILE *fp = fopen(gl.gl_pathv[j], "r");
		size_t mod_len = 0;
		char *mod = NULL;

		if (!fp) {
			ULOG_ERR("failed to open %s\n", gl.gl_pathv[j]);
			continue;
		}

		while (getline(&mod, &mod_len, fp) > 0) {
			char *nl = strchr(mod, '\n');
			struct module *m;
			char *opts;

			if (nl)
				*nl = '\0';

			opts = strchr(mod, ' ');
			if (opts)
				*opts++ = '\0';

			m = find_module(get_module_name(mod));
			if (!m || (m->state == LOADED))
				continue;

			if (opts)
				m->opts = strdup(opts);
			m->state = PROBE;
			if (basename(gl.gl_pathv[j])[0] - '0' <= 9)
				load_modprobe();

		}
		free(mod);
		fclose(fp);
	}

	fail = load_modprobe();

	if (fail) {
		ULOG_ERR("%d module%s could not be probed\n",
		         fail, (fail == 1) ? ("") : ("s"));

		avl_for_each_element(&modules, mn, avl) {
			if (mn->is_alias)
				continue;
			m = mn->m;
			if ((m->state == PROBE) || (m->error))
				ULOG_ERR("- %s - %d\n", m->name, deps_available(m, 1));
		}
	} else {
		ULOG_INFO("done loading kernel modules from %s\n", path);
	}

out:
	globfree(&gl);
	free(path);

	return 0;
}

static inline char weight(char c)
{
	return c == '_' ? '-' : c;
}

static int avl_modcmp(const void *k1, const void *k2, void *ptr)
{
	const char *s1 = k1;
	const char *s2 = k2;

	while (*s1 && (weight(*s1) == weight(*s2)))
	{
		s1++;
		s2++;
	}

	return (unsigned char)weight(*s1) - (unsigned char)weight(*s2);
}

int main(int argc, char **argv)
{
	char *exec = basename(*argv);

	avl_init(&modules, avl_modcmp, false, NULL);
	if (!strcmp(exec, "insmod"))
		return main_insmod(argc, argv);

	if (!strcmp(exec, "rmmod"))
		return main_rmmod(argc, argv);

	if (!strcmp(exec, "lsmod"))
		return main_lsmod(argc, argv);

	if (!strcmp(exec, "modinfo"))
		return main_modinfo(argc, argv);

	if (!strcmp(exec, "modprobe"))
		return main_modprobe(argc, argv);

	ulog_open(ULOG_KMSG, LOG_USER, "kmodloader");
	return main_loader(argc, argv);
}
