import os

loaded = {}
main_map = None
secure = False

def has_slash(name):
    return '/' in name

def dl_dst_lib():
    return DL_DST_LIB

def dl_platform():
    return GLRO_DL_PLATFORM

def _dst_value(name, obj):
    if name == 'ORIGIN':
        return os.path.dirname(os.path.realpath(obj.path))
    if name == 'LIB':
        return dl_dst_lib()
    if name == 'PLATFORM':
        return dl_platform()
    return None

def expand_dynamic_string(s, obj):
    out = []
    i = 0
    n = len(s)
    while i < n:
        if s[i] != '$':
            out.append(s[i]); i += 1; continue
        if i + 1 < n and s[i+1] == '{':
            j = s.find('}', i + 2)
            if j == -1:
                out.append(s[i:]); break
            val = _dst_value(s[i+2:j], obj)
            out.append(val if val is not None else s[i:j+1])
            i = j + 1
            continue
        j = i + 1
        while j < n and (s[j].isalnum() or s[j] == '_'):
            j += 1
        name = s[i+1:j]
        if name:
            val = _dst_value(name, obj)
            out.append(val if val is not None else s[i:j])
            i = j
        else:
            out.append('$'); i += 1
    return ''.join(out)

def split_dirs(s, obj):
    return [expand_dynamic_string(p if p else '.', obj)
            for p in s.split(':')]

def name_matches(obj, name):
    return name in obj.libname

def version_problem(dependent, dep_name, dep_obj):
    for v in dependent.verneed.get(dep_name, []):
        if v not in dep_obj.verdef:
            return v
    return None

def map_object(path, opened_as, loader=None):
    st = os.stat(path)
    path = os.path.realpath(path)

    for obj in loaded.values():
        if (obj.dev, obj.ino) == (st.st_dev, st.st_ino):
            if opened_as not in obj.libname:
                obj.libname.append(opened_as)
            return obj

    obj = load_elf(path)
    obj.path = path
    obj.dev, obj.ino = st.st_dev, st.st_ino
    obj.libname = [opened_as]
    if obj.soname:
        obj.libname.append(obj.soname)
    obj.deps = {}
    obj.loader = loader

    loaded[path] = obj
    return obj

def bind(dependent, dep_name, dep_obj):
    missing = version_problem(dependent, dep_name, dep_obj)
    if missing is not None:
        if not dep_obj.verdef:
            warn("{}: no version information available (required by {})".format(
                dep_obj.path, dependent.path))
        else:
            raise FatalError("{}: version `{}' not found (required by {})".format(
                dep_obj.path, missing, dependent.path))
    dependent.deps[dep_name] = dep_obj

def rpath_chain(loader):
    dirs = []
    l = loader
    while l is not None:
        if l.rpath and not l.runpath:
            dirs += split_dirs(l.rpath, l)
        l = l.loader
    return dirs

def search_dirs_for(loader):
    actions = []

    if loader is None or not loader.runpath:
        d = rpath_chain(loader)
        if d:
            actions.append(('rpath', d))

    if not secure:
        env = os.environ.get('LD_LIBRARY_PATH', '')
        if env:
            dst_base = main_map if main_map is not None else loader
            actions.append(('env', split_dirs(env, dst_base)))

    if loader is not None and loader.runpath:
        actions.append(('runpath', split_dirs(loader.runpath, loader)))

    actions.append(('cache', None))

    actions.append(('default', ['/lib/x86_64-linux-gnu', '/usr/lib/x86_64-linux-gnu',
                                '/lib', '/usr/lib']))
    return actions

def _file_magic(path):
    with open(path, 'rb') as f:
        return f.read(4)

def find_file(name, dirs):
    for d in dirs:
        p = os.path.join(d, name)
        if not os.path.exists(p):
            continue
        if os.path.isdir(p):
            raise FatalError("{}: cannot read file data: Error 21".format(p))
        if not os.access(p, os.R_OK):
            continue
        if os.stat(p).st_size < 64:
            raise FatalError("{}: file too short".format(p))
        if _file_magic(p) != b'\x7fELF':
            raise FatalError("{}: invalid ELF header".format(p))
        return p
    return None

def _verify_candidate(p):
    if os.path.isdir(p):
        raise FatalError("{}: cannot read file data: Error 21".format(p))
    if os.stat(p).st_size < 64:
        raise FatalError("{}: file too short".format(p))
    if _file_magic(p) != b'\x7fELF':
        raise FatalError("{}: invalid ELF header".format(p))

def resolve(dependent, dep_name):
    loader = dependent

    for obj in loaded.values():
        if name_matches(obj, dep_name):
            bind(dependent, dep_name, obj)
            return obj

    if has_slash(dep_name):
        p = expand_dynamic_string(dep_name, dependent)
        if not os.path.exists(p):
            raise LibraryNotFound(p)
        _verify_candidate(p)
        dep_obj = map_object(p, opened_as=p, loader=dependent)
        bind(dependent, dep_name, dep_obj)
        return dep_obj

    name = expand_dynamic_string(dep_name, dependent)

    path = None
    for source, dirs in search_dirs_for(loader):
        if source == 'cache':
            cand = ld_so_cache().get(name)
            if cand is None:
                continue
            if not os.path.exists(cand) or not os.access(cand, os.R_OK):
                continue
            _verify_candidate(cand)
            path = cand
        else:
            path = find_file(name, dirs)
        if path:
            break

    if not path:
        raise LibraryNotFound(name)

    dep_obj = map_object(path, opened_as=name, loader=dependent)
    bind(dependent, dep_name, dep_obj)
    return dep_obj

def resolve_dependencies(roots):
    queue = []
    queued = set()
    for obj in roots:
        if id(obj) not in queued:
            queued.add(id(obj))
            queue.append(obj)
    while queue:
        obj = queue.pop(0)
        for dep_name in obj.needed:
            if dep_name in obj.deps:
                continue
            dep_obj = resolve(obj, dep_name)
            if id(dep_obj) not in queued:
                queued.add(id(dep_obj))
                queue.append(dep_obj)

def handle_preload_list(env):
    if secure:
        return []
    preloaded = []
    for item in env.get('LD_PRELOAD', '').replace(':', ' ').split():
        try:
            if has_slash(item):
                p = expand_dynamic_string(item, main_map)
                if not os.path.exists(p):
                    warn("object '{}' from LD_PRELOAD cannot be preloaded "
                         "(cannot open shared object file): ignored".format(item))
                    continue
                _verify_candidate(p)
                path = p
            else:
                name = expand_dynamic_string(item, main_map)
                path = None
                for source, dirs in search_dirs_for(main_map):
                    if source == 'cache':
                        cand = ld_so_cache().get(name)
                        if cand is not None and os.path.exists(cand) \
                           and os.access(cand, os.R_OK):
                            _verify_candidate(cand)
                            path = cand
                    else:
                        path = find_file(name, dirs)
                    if path:
                        break
                if not path:
                    warn("object '{}' from LD_PRELOAD cannot be preloaded "
                         "(cannot open shared object file): ignored".format(item))
                    continue
            preloaded.append(map_object(path, opened_as=item, loader=main_map))
        except FatalError as e:
            warn("object '{}' from LD_PRELOAD cannot be preloaded ({}): ignored".format(
                item, str(e).split(': ', 1)[-1]))
    return preloaded

def startup(exe_path, env):
    global main_map
    main_map = map_object(exe_path, opened_as=exe_path)
    preloaded = handle_preload_list(env)
    resolve_dependencies([main_map] + preloaded)
    return main_map

RTLD_LAZY = 1
RTLD_NOW = 2

def dlopen(name, caller, flags=None):
    snapshot = set(loaded)
    try:
        for obj in loaded.values():
            if name_matches(obj, name):
                return obj
        if has_slash(name):
            p = expand_dynamic_string(name, caller)
            if not os.path.exists(p):
                return None
            _verify_candidate(p)
            obj = map_object(p, opened_as=p, loader=caller)
        else:
            obj = resolve(caller, name)
        resolve_dependencies([obj])
        if flags is not None and flags & RTLD_NOW:
            defined = set()
            for o in loaded.values():
                defined |= set(getattr(o, 'defines', None) or ())
            for k in loaded:
                if k in snapshot:
                    continue
                for sym in getattr(loaded[k], 'undefined', None) or ():
                    if sym not in defined:
                        raise FatalError("{}: undefined symbol: {}".format(k, sym))
        return obj
    except (LibraryNotFound, FatalError):
        for k in [k for k in loaded if k not in snapshot]:
            del loaded[k]
        return None
