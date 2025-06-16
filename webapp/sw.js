// Simple service worker for Alive Light Control
// Minimal caching strategy to avoid storage issues

const CACHE_NAME = 'alive-light-v1';

// Only cache essential files
const urlsToCache = [
    '/',
    '/index.html'
];

// Install event - minimal caching
self.addEventListener('install', event => {
    console.log('[SW] Installing service worker');
    event.waitUntil(
        caches.open(CACHE_NAME)
            .then(cache => {
                console.log('[SW] Caching essential files');
                return cache.addAll(urlsToCache).catch(err => {
                    console.warn('[SW] Failed to cache some files:', err);
                    // Don't fail if caching fails
                    return Promise.resolve();
                });
            })
            .catch(err => {
                console.warn('[SW] Cache setup failed:', err);
                // Don't fail the install
                return Promise.resolve();
            })
    );
    
    // Skip waiting to activate immediately
    self.skipWaiting();
});

// Activate event - clean up old caches
self.addEventListener('activate', event => {
    console.log('[SW] Activating service worker');
    event.waitUntil(
        caches.keys().then(cacheNames => {
            return Promise.all(
                cacheNames.map(cacheName => {
                    if (cacheName !== CACHE_NAME) {
                        console.log('[SW] Deleting old cache:', cacheName);
                        return caches.delete(cacheName);
                    }
                })
            );
        }).catch(err => {
            console.warn('[SW] Cache cleanup failed:', err);
            return Promise.resolve();
        })
    );
    
    // Take control of all pages immediately
    self.clients.claim();
});

// Fetch event - network first, then cache
self.addEventListener('fetch', event => {
    // Only handle GET requests
    if (event.request.method !== 'GET') {
        return;
    }
    
    // Skip non-HTTP requests
    if (!event.request.url.startsWith('http')) {
        return;
    }
    
    event.respondWith(
        fetch(event.request)
            .then(response => {
                // If network request succeeds, return it
                return response;
            })
            .catch(() => {
                // If network fails, try cache
                return caches.match(event.request)
                    .then(response => {
                        if (response) {
                            console.log('[SW] Serving from cache:', event.request.url);
                            return response;
                        }
                        // If not in cache either, return a basic response
                        return new Response('Offline - Please check your connection', {
                            status: 503,
                            statusText: 'Service Unavailable'
                        });
                    });
            })
    );
});