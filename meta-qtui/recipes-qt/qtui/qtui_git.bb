require qtui.inc

PV = "0.2.0"

S = "${WORKDIR}/git"
B = "${WORKDIR}/build"

do_unpack[nostamp] = "1"
do_configure[nostamp] = "1"

python do_unpack() {
    import shutil
    import os
    
    src_dir = os.path.abspath(d.getVar('THISDIR') + '/../../..')
    s_dir = d.getVar('S')
    
    if os.path.exists(s_dir):
        shutil.rmtree(s_dir)
    os.makedirs(s_dir, exist_ok=True)
    
    for item in ['src', 'docs', 'cmake', 'CMakeLists.txt', 'LICENSE']:
        src_path = os.path.join(src_dir, item)
        dst_path = os.path.join(s_dir, item)
        if os.path.isdir(src_path):
            shutil.copytree(src_path, dst_path, symlinks=True)
        elif os.path.isfile(src_path):
            shutil.copy2(src_path, dst_path)
}
