document.addEventListener("DOMContentLoaded", () => {

    const editor = document.getElementById("editor-top");
    const consola = document.getElementById("editor-bottom");

    const btnNuevo = document.getElementById("nuevo-archivo");
    const btnCargar = document.getElementById("cargar-archivo");
    const btnGuardar = document.getElementById("guardar-archivo");
    const btnLimpiarConsola = document.getElementById("limpiar-consola");

    /* ===============================
       Nuevo archivo / Limpiar todo
       =============================== */
    btnNuevo.addEventListener("click", (e) => {
        e.preventDefault();

        if (editor.value.trim() !== "" || consola.value.trim() !== "") {
            const confirmar = confirm(
                "Hay contenido en el editor o en la consola.\n¿Desea crear un nuevo archivo y limpiar todo?"
            );
            if (!confirmar) return;
        }

        editor.value = "";
        consola.value = "";
    });

    /* ===============================
       Cargar archivo .txt
       =============================== */
    btnCargar.addEventListener("click", (e) => {
        e.preventDefault();

        if (editor.value.trim() !== "") {
            const confirmar = confirm(
                "El editor contiene texto.\n¿Desea reemplazarlo al cargar un archivo?"
            );
            if (!confirmar) return;
        }

        const input = document.createElement("input");
        input.type = "file";
        input.accept = ".txt";

        input.addEventListener("change", () => {
            const file = input.files[0];
            if (!file) return;

            const reader = new FileReader();
            reader.onload = () => {
                editor.value = reader.result;
            };
            reader.readAsText(file);
        });

        input.click();
    });

    /* ===============================
       Guardar archivo .txt
       =============================== */
    btnGuardar.addEventListener("click", (e) => {
        e.preventDefault();

        if (editor.value.trim() === "") {
            alert("No hay contenido para guardar.");
            return;
        }

        let nombre = prompt("Ingrese el nombre del archivo:", "codigo.txt");
        if (!nombre) return;

        if (!nombre.endsWith(".txt")) {
            nombre += ".txt";
        }

        const blob = new Blob([editor.value], { type: "text/plain" });
        const url = URL.createObjectURL(blob);

        const a = document.createElement("a");
        a.href = url;
        a.download = nombre;
        document.body.appendChild(a);
        a.click();

        document.body.removeChild(a);
        URL.revokeObjectURL(url);
    });

    /* ===============================
       Limpiar consola
       =============================== */
    btnLimpiarConsola.addEventListener("click", (e) => {
        e.preventDefault();

        if (consola.value.trim() !== "") {
            const confirmar = confirm(
                "La consola contiene texto.\n¿Desea limpiarla?"
            );
            if (!confirmar) return;
        }

        consola.value = "";
    });

});