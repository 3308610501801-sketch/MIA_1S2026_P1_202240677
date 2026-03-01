document.addEventListener("DOMContentLoaded", function () {

    const botonAnalisis = document.getElementById("ejecutar-analisis");

    const editorTop = document.getElementById("editor-top");
    const consola = document.getElementById("editor-bottom");

    let erroresActuales = [];
    let tablaSimbolosActual = [];

    botonAnalisis.addEventListener("click", function (e) {
        e.preventDefault();

        const codigo = editorTop.value;

        if (codigo.trim() === "") {
            consola.value = "No hay texto para analizar.";
            return;
        }

        consola.value = "Ejecutando análisis...\n";

        fetch("/analizar", {
            method: "POST",
            headers: {
                "Content-Type": "text/plain"
            },
            body: codigo
        })
        .then(response => {
            if (!response.ok) {
                throw new Error("Error HTTP: " + response.status);
            }
            return response.text();
        })
        .then(data => {
            consola.value = data;

            erroresActuales = [];
            tablaSimbolosActual = [];
        })
        .catch(error => {
            consola.value = "Error al ejecutar el análisis:\n" + error;
            erroresActuales = [];
            tablaSimbolosActual = [];
        });
    });

});