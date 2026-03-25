function Component(){
}

Component.prototype.createOperations = function(){
    // data dir contains bin/kmiller and desktop file
    component.createOperations();

    var ver = "@APP_VERSION@";
    var base = "/opt/kmiller";
    var vers = base + "/versions/" + ver;

    // Ensure dirs
    component.addOperation("Mkdir", base);
    component.addOperation("Mkdir", base + "/versions");
    component.addOperation("Mkdir", vers + "/bin");
    component.addOperation("Mkdir", "/usr/local/bin");

    // Copy binary
    component.addOperation("Copy", "@TargetDir@/bin/kmiller", vers + "/bin/kmiller");
    component.addOperation("Execute", "chmod", "755", vers + "/bin/kmiller");

    // Update stable launcher symlink
    component.addOperation("Execute", "ln", "-sfn", vers + "/bin/kmiller", "/usr/local/bin/kmiller");

    // Desktop entry
    component.addOperation("Mkdir", "/usr/local/share/applications");
    component.addOperation("Copy", "@TargetDir@/share/applications/kmiller.desktop", "/usr/local/share/applications/kmiller.desktop");

    // Refresh KDE caches (best-effort)
    component.addOperation("Execute", "bash", "-lc", "kbuildsycoca6 || true; update-desktop-database /usr/local/share/applications || true");
};
