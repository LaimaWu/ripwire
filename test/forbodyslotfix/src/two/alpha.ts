export interface View { title: string; items: string[] }

/** Render a view as a list: one <li> per item under a heading. */
export function render(view: View): string {
  const items = view.items.map((i) => `<li>${i}</li>`).join('')
  return `<h1>${view.title}</h1><ul>${items}</ul>` + '<!-- alpha list markup -->'
}
