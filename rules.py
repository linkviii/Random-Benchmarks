from pathlib import Path
from pprint import pprint

def stringify(l):
    return [str(it) for it in l]

def prefix(l, pre):
    return [f"{pre}{it}" for it in l]



def rules(ctx):
    def add_rule(o_file, i_files:list, cmd, d_file=None):
        if d_file:
            ctx.add_rule(str(o_file), stringify(i_files), stringify(cmd), d_file=str(d_file))
        else:
            ctx.add_rule(str(o_file), stringify(i_files), stringify(cmd))



    o_list = []

    out_dir = Path("./_out")


    libs = ["fmt","benchmark", "pthread"]
    libs = prefix(libs, "-l")
    cxx_ver = "-std=c++17"
    link_flags = [cxx_ver]
    cxx_flags = [
        cxx_ver,
        "-MD",
        "-O2",
        '-Wall', '-Werror=return-type', '-Wcast-align'
    ]
    # Ensure color even with python's pipes
    cxx_flags.append('-fdiagnostics-color=always')    

    src = Path("src").glob("*.cpp")

    for f in src:
        out = out_dir / f.with_suffix(".o").name
        d = out_dir / f.with_suffix(".d").name
        cmd = ["g++", "-o", out, "-c", f] + cxx_flags   + libs      

        o_list.append(out)
        add_rule(out, [f], cmd, d_file=d)

    main = out_dir / "main"
    cmd = ["g++" , "-o", main] + o_list + libs + link_flags
    add_rule(main, o_list, cmd)

    all = o_list + [main]
    add_rule('all', all, ['echo'])
    pprint(all)

