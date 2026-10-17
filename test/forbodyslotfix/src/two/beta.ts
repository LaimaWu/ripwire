import type { View } from './alpha'

/** Render a view as a table: one <tr> per item under a caption. */
export function render(view: View): string {
  const rows = view.items.map((i) => `<tr><td>${i}</td></tr>`).join('')
  return `<table><caption>${view.title}</caption>${rows}</table>` + '<!-- beta table markup -->'
}
