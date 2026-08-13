---
title: EPUB Indexing Methods
nav_order: 8.5
---

# EPUB Indexing Methods

Before CrossSmudge can display an EPUB chapter, it lays out the chapter into
pages and saves that layout in the book's cache. **Indexing Method** chooses
whether CrossSmudge finishes that work before you start reading the chapter, or
does it a little at a time as you read.

The default is **Full Section**. Most books work well with it. Choose
**Incremental** for a book with unusually large chapters, or when waiting for
an entire chapter to open is more disruptive than seeing a short indexing wait
later.

| Method       | What happens                                                                                                | Main benefit                                                    | Main tradeoff                                                                                                |
| ------------ | ----------------------------------------------------------------------------------------------------------- | --------------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------ |
| Full Section | Builds and caches the whole chapter before it is shown.                                                     | Normal page turns within the chapter do not need more indexing. | A large uncached chapter can take a noticeable time to open.                                                 |
| Incremental  | Builds enough pages to show your current position, then continues in small background steps while you read. | You can begin reading a large chapter sooner.                   | If you reach pages that have not been built yet, CrossSmudge may briefly show **Indexing** while it catches up. |

## Full Section

Full Section finishes the complete chapter cache before CrossSmudge displays an
uncached chapter. This is the simpler, more predictable experience for normal
EPUBs: after the initial wait, the chapter's page count and all of its pages
are ready.

CrossSmudge also tries to build the next chapter while you are reading the
penultimate page of the current one. When this succeeds, moving to the next
chapter does not require a visible indexing wait. It is deliberately
best-effort: on the X3/X4's limited memory, CrossSmudge skips that background work
when there is not enough free or contiguous memory, and indexes the next
chapter when you enter it instead.

Use Full Section when:

- Most chapters open quickly and you want the smoothest page turns.
- You prefer any indexing wait to happen at the chapter boundary.
- You want a complete chapter cache and final page count as soon as it opens.

Consider Incremental when a particular chapter takes a long time to open, or
when a very large chapter is more likely to stress the reader's available
memory.

## Incremental

Incremental builds only as far as CrossSmudge needs to display the current page.
It keeps a small number of pages ready ahead of your reading position and uses
short background steps to extend that ready area. Think of it like loading the
first screen of a long web page first, then preparing the rest while you read.

This avoids making you wait for every page in a very large chapter before you
can start. CrossSmudge saves the pages it has already built, so leaving the book
does not discard that readable progress. A giant chapter may therefore remain
partially indexed until you read farther into it; it does not have to finish in
one continuous session.

Use Incremental when:

- A book pauses for a long time at **Indexing** before its first page appears.
- The book has exceptionally large chapters, such as omnibus editions or
  poorly split EPUBs.
- Starting to read sooner matters more than avoiding an occasional wait later.

Expect a visible **Indexing** popup if you jump far ahead, follow a link to an
unbuilt part of the chapter, or turn pages faster than the background work can
stay ahead. That is normal: CrossSmudge is building just enough additional pages
to make the requested position readable.

KOReader Sync uses the same content location rather than the other device's
page number. If a synced location is beyond this device's saved incremental
prefix, CrossSmudge indexes forward until that location is available. Switching
between Incremental and Full Section does not change the saved reading location:
Full Section resolves it as soon as the chapter is built, while Incremental only
builds through the requested content.

## Changing The Setting

To change the default for future EPUBs, open **Settings → Reader → Indexing
Method** and choose **Incremental** or **Full Section**.

To change it only for the EPUB you are reading, open the reader menu, choose
**Reader Options**, then choose **Indexing Method**. The per-book choice is
saved with that book and overrides the global default without changing your
other books.

Changing the method does not discard the chapter currently on screen. The new
choice is used the next time CrossSmudge needs to index a chapter. It also does
not alter the EPUB file itself, your reading progress, bookmarks, clippings,
or reading statistics.

## If A Book Is Still Slow Or Cannot Index

Indexing Method changes _when_ CrossSmudge performs layout work; it does not
simplify the publisher's CSS, images, or tables. If a difficult EPUB is still
slow or runs out of memory, try a lighter [EPUB Render Mode](./epub-render-modes.md)
or [optimize](https://inky.crosssmudge.dev) the EPUB before copying it to the device.
