(function(){
  const gutter = document.getElementById('gutter');
  const top = document.getElementById('top-panel');
  const bottom = document.getElementById('bottom-panel');
  const container = document.querySelector('.entorno_trabajo');

  let dragging = false;
  let startY = 0;
  let startTopHeight = 0;

  gutter.addEventListener('pointerdown', (e) => {
    dragging = true;
    startY = e.clientY;
    startTopHeight = top.getBoundingClientRect().height;
    gutter.setPointerCapture(e.pointerId);
    document.body.style.cursor = 'row-resize';
  });

  window.addEventListener('pointermove', (e) => {
    if (!dragging) return;
    const dy = e.clientY - startY;
    const containerHeight = container.getBoundingClientRect().height;
    let newTopHeight = startTopHeight + dy;

    const min = 50;
    const max = containerHeight - min - gutter.offsetHeight;
    if (newTopHeight < min) newTopHeight = min;
    if (newTopHeight > max) newTopHeight = max;

    top.style.flex = `0 0 ${newTopHeight}px`;
    bottom.style.flex = '1 1 0%';
  });

  window.addEventListener('pointerup', (e) => {
    dragging = false;
    try { gutter.releasePointerCapture(e.pointerId); } catch(_) {}
    document.body.style.cursor = '';
  });

  gutter.addEventListener('dblclick', () => {
    top.style.flex = '1 1 0%';
    bottom.style.flex = '1 1 0%';
  });
})();


function setupEditor(textareaId, numbersId) {
  const textarea = document.getElementById(textareaId);
  const numbers = document.getElementById(numbersId);

  function updateLineNumbers() {
    const lines = textarea.value.split('\n').length;
    let text = '';
    for (let i = 1; i <= lines; i++) {
      text += i + '\n';
    }
    numbers.textContent = text;
  }

  // Actualizar al escribir
  textarea.addEventListener('input', updateLineNumbers);

  // Sincronizar scroll
  textarea.addEventListener('scroll', () => {
    numbers.scrollTop = textarea.scrollTop;
  });

  // Inicial
  updateLineNumbers();
}

// Inicializamos ambos paneles
setupEditor('editor-top', 'line-numbers-top');
setupEditor('editor-bottom', 'line-numbers-bottom');
