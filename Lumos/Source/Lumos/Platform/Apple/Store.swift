// StoreKit 2 behind a small C interface (Core/OS/Store.cpp). StoreKit 2 is Swift-only.
// Strings cross as UTF-8: fields split by 0x1f, records by 0x1e; returned strings are malloc'd.
import Foundation
import StoreKit
#if os(iOS)
import UIKit
#endif

private let fieldSep = "\u{1f}"
private let recordSep = "\u{1e}"

@available(macOS 12.0, iOS 15.0, *)
private final class LumosStore {
    static let shared = LumosStore()

    private let lock = NSLock()
    private var products: [String: Product] = [:]
    private var order: [String] = []
    private var loaded = false
    private var events: [String] = []
    private var entitled: Set<String> = []
    private var updates: Task<Void, Never>?

    private init() {
        // purchases made elsewhere (another device, Ask to Buy approvals, refunds)
        updates = Task.detached { [weak self] in
            for await result in Transaction.updates {
                await self?.handle(result)
            }
        }
        Task { await self.refreshEntitlements() }
    }

    private func locked<T>(_ body: () -> T) -> T {
        lock.lock()
        defer { lock.unlock() }
        return body()
    }

    func push(_ kind: String, _ id: String = "", _ message: String = "") {
        let clean = message.replacingOccurrences(of: fieldSep, with: " ").replacingOccurrences(of: recordSep, with: " ")
        locked { events.append([kind, id, clean].joined(separator: fieldSep)) }
    }

    func handle(_ result: VerificationResult<Transaction>) async {
        guard case .verified(let t) = result else {
            push("failed", "", "The purchase couldn't be verified")
            return
        }
        if t.productType != .consumable {
            locked {
                if t.revocationDate == nil { entitled.insert(t.productID) } else { entitled.remove(t.productID) }
            }
            push("entitlements")
        }
        if t.revocationDate == nil { push("purchased", t.productID) }
        await t.finish()
    }

    func refreshEntitlements() async {
        var owned = Set<String>()
        for await r in Transaction.currentEntitlements {
            if case .verified(let t) = r, t.revocationDate == nil { owned.insert(t.productID) }
        }
        locked { entitled = owned }
        push("entitlements")
    }

    func request(_ ids: [String]) {
        Task {
            do {
                let list = try await Product.products(for: ids)
                locked {
                    for p in list { products[p.id] = p }
                    order = ids.filter { products[$0] != nil }
                    loaded = true
                }
                push("products")
            } catch {
                locked { loaded = true }
                push("failed", "", error.localizedDescription)
            }
        }
    }

    // the storefront's own price format and symbol, but western digits (the UI font has no
    // Arabic-Indic / Devanagari digits); plus "SAR 36.99" for symbols the font can't draw
    private func prices(_ p: Product) -> (String, String) {
        let style = p.priceFormatStyle
        let base = style.locale.identifier
        let latn = Locale(identifier: base.contains("@") ? base + ";numbers=latn" : base + "@numbers=latn")
        let shown = p.price.formatted(style.locale(latn))
        let plain = style.currencyCode + " " + p.price.formatted(.number.precision(.fractionLength(0...2)).locale(Locale(identifier: "en_US_POSIX")))
        return (shown, plain)
    }

    func productList() -> String? {
        locked {
            guard loaded else { return nil }
            return order.compactMap { id -> String? in
                guard let p = products[id] else { return nil }
                let kind = p.type == .consumable ? "consumable" : "nonconsumable"
                let (shown, plain) = prices(p)
                return [p.id, p.displayName, p.description, shown, kind, plain]
                    .map { $0.replacingOccurrences(of: fieldSep, with: " ").replacingOccurrences(of: recordSep, with: " ") }
                    .joined(separator: fieldSep)
            }.joined(separator: recordSep)
        }
    }

    func buy(_ id: String) {
        Task {
            guard let p = locked({ products[id] }) else {
                push("failed", id, "The store isn't available right now")
                return
            }
            do {
                switch try await p.purchase() {
                case .success(let v): await handle(v)
                case .userCancelled: push("cancelled", id)
                case .pending: push("pending", id)
                @unknown default: push("failed", id, "Unknown result")
                }
            } catch {
                push("failed", id, error.localizedDescription)
            }
        }
    }

    func restore() {
        Task {
            do {
                try await AppStore.sync()
            } catch {
                push("failed", "", error.localizedDescription)
            }
            await refreshEntitlements()
            push("restored")
        }
    }

    func isEntitled(_ id: String) -> Bool { locked { entitled.contains(id) } }

    func poll() -> String? {
        locked {
            if events.isEmpty { return nil }
            let s = events.joined(separator: recordSep)
            events.removeAll()
            return s
        }
    }
}

private func dup(_ s: String?) -> UnsafeMutablePointer<CChar>? {
    guard let s = s else { return nil }
    return strdup(s)
}

@_cdecl("LumosStoreSupported")
public func LumosStoreSupported() -> Bool {
    if #available(macOS 12.0, iOS 15.0, *) { return true }
    return false
}

@_cdecl("LumosStoreRequest")
public func LumosStoreRequest(_ ids: UnsafePointer<CChar>) {
    guard #available(macOS 12.0, iOS 15.0, *) else { return }
    let list = String(cString: ids).components(separatedBy: fieldSep).filter { !$0.isEmpty }
    LumosStore.shared.request(list)
}

@_cdecl("LumosStoreProducts")
public func LumosStoreProducts() -> UnsafeMutablePointer<CChar>? {
    guard #available(macOS 12.0, iOS 15.0, *) else { return nil }
    return dup(LumosStore.shared.productList())
}

@_cdecl("LumosStoreBuy")
public func LumosStoreBuy(_ id: UnsafePointer<CChar>) {
    guard #available(macOS 12.0, iOS 15.0, *) else { return }
    LumosStore.shared.buy(String(cString: id))
}

@_cdecl("LumosStoreRestore")
public func LumosStoreRestore() {
    guard #available(macOS 12.0, iOS 15.0, *) else { return }
    LumosStore.shared.restore()
}

@_cdecl("LumosStoreEntitled")
public func LumosStoreEntitled(_ id: UnsafePointer<CChar>) -> Bool {
    guard #available(macOS 12.0, iOS 15.0, *) else { return false }
    return LumosStore.shared.isEntitled(String(cString: id))
}

@_cdecl("LumosStorePoll")
public func LumosStorePoll() -> UnsafeMutablePointer<CChar>? {
    guard #available(macOS 12.0, iOS 15.0, *) else { return nil }
    return dup(LumosStore.shared.poll())
}

@_cdecl("LumosStoreRequestReview")
public func LumosStoreRequestReview() {
    DispatchQueue.main.async {
        #if os(iOS)
        let scene = UIApplication.shared.connectedScenes
            .first { $0.activationState == .foregroundActive } as? UIWindowScene
        if let scene = scene {
            if #available(iOS 16.0, *) {
                AppStore.requestReview(in: scene)
            } else {
                SKStoreReviewController.requestReview(in: scene)
            }
        }
        #else
        SKStoreReviewController.requestReview()
        #endif
    }
}

@_cdecl("LumosAppVersion")
public func LumosAppVersion() -> UnsafeMutablePointer<CChar>? {
    let info = Bundle.main.infoDictionary ?? [:]
    let short = info["CFBundleShortVersionString"] as? String ?? ""
    let build = info["CFBundleVersion"] as? String ?? ""
    if short.isEmpty { return dup(build) }
    return dup(build.isEmpty || build == short ? short : "\(short) (\(build))")
}
