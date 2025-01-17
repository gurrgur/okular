/*
    SPDX-FileCopyrightText: 2018 Chinmoy Ranjan Pradhan <chinmoyrp65@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "signatureguiutils.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QInputDialog>
#include <QMimeDatabase>

#include <KLocalizedString>
#include <KMessageBox>

#include "core/document.h"
#include "core/form.h"
#include "core/page.h"
#include "pageview.h"

namespace SignatureGuiUtils
{
QVector<const Okular::FormFieldSignature *> getSignatureFormFields(Okular::Document *doc)
{
    uint curPage = 0;
    const uint endPage = doc->pages() - 1;
    QVector<const Okular::FormFieldSignature *> signatureFormFields;
    while (curPage <= endPage) {
        const QLinkedList<Okular::FormField *> formFields = doc->page(curPage++)->formFields();
        for (Okular::FormField *f : formFields) {
            if (f->type() == Okular::FormField::FormSignature) {
                signatureFormFields.append(static_cast<Okular::FormFieldSignature *>(f));
            }
        }
    }
    std::sort(signatureFormFields.begin(), signatureFormFields.end(), [](const Okular::FormFieldSignature *a, const Okular::FormFieldSignature *b) {
        const Okular::SignatureInfo &infoA = a->signatureInfo();
        const Okular::SignatureInfo &infoB = b->signatureInfo();
        return infoA.signingTime() < infoB.signingTime();
    });
    return signatureFormFields;
}

QString getReadableSignatureStatus(Okular::SignatureInfo::SignatureStatus sigStatus)
{
    switch (sigStatus) {
    case Okular::SignatureInfo::SignatureValid:
        return i18n("The signature is cryptographically valid.");
    case Okular::SignatureInfo::SignatureInvalid:
        return i18n("The signature is cryptographically invalid.");
    case Okular::SignatureInfo::SignatureDigestMismatch:
        return i18n("Digest Mismatch occurred.");
    case Okular::SignatureInfo::SignatureDecodingError:
        return i18n("The signature CMS/PKCS7 structure is malformed.");
    case Okular::SignatureInfo::SignatureNotFound:
        return i18n("The requested signature is not present in the document.");
    default:
        return i18n("The signature could not be verified.");
    }
}

QString getReadableCertStatus(Okular::SignatureInfo::CertificateStatus certStatus)
{
    switch (certStatus) {
    case Okular::SignatureInfo::CertificateTrusted:
        return i18n("Certificate is Trusted.");
    case Okular::SignatureInfo::CertificateUntrustedIssuer:
        return i18n("Certificate issuer isn't Trusted.");
    case Okular::SignatureInfo::CertificateUnknownIssuer:
        return i18n("Certificate issuer is unknown.");
    case Okular::SignatureInfo::CertificateRevoked:
        return i18n("Certificate has been Revoked.");
    case Okular::SignatureInfo::CertificateExpired:
        return i18n("Certificate has Expired.");
    case Okular::SignatureInfo::CertificateNotVerified:
        return i18n("Certificate has not yet been verified.");
    default:
        return i18n("Unknown issue with Certificate or corrupted data.");
    }
}

QString getReadableHashAlgorithm(Okular::SignatureInfo::HashAlgorithm hashAlg)
{
    switch (hashAlg) {
    case Okular::SignatureInfo::HashAlgorithmMd2:
        return i18n("MD2");
    case Okular::SignatureInfo::HashAlgorithmMd5:
        return i18n("MD5");
    case Okular::SignatureInfo::HashAlgorithmSha1:
        return i18n("SHA1");
    case Okular::SignatureInfo::HashAlgorithmSha256:
        return i18n("SHA256");
    case Okular::SignatureInfo::HashAlgorithmSha384:
        return i18n("SHA384");
    case Okular::SignatureInfo::HashAlgorithmSha512:
        return i18n("SHA512");
    case Okular::SignatureInfo::HashAlgorithmSha224:
        return i18n("SHA224");
    default:
        return i18n("Unknown Algorithm");
    }
}

QString getReadablePublicKeyType(Okular::CertificateInfo::PublicKeyType type)
{
    switch (type) {
    case Okular::CertificateInfo::RsaKey:
        return i18n("RSA");
    case Okular::CertificateInfo::DsaKey:
        return i18n("DSA");
    case Okular::CertificateInfo::EcKey:
        return i18n("EC");
    case Okular::CertificateInfo::OtherKey:
        return i18n("Unknown Type");
    }

    return i18n("Unknown Type");
}

QString getReadableKeyUsage(Okular::CertificateInfo::KeyUsageExtensions kuExtensions, const QString &separator)
{
    QStringList ku;
    if (kuExtensions.testFlag(Okular::CertificateInfo::KuDigitalSignature))
        ku << i18n("Digital Signature");
    if (kuExtensions.testFlag(Okular::CertificateInfo::KuNonRepudiation))
        ku << i18n("Non-Repudiation");
    if (kuExtensions.testFlag(Okular::CertificateInfo::KuKeyEncipherment))
        ku << i18n("Encrypt Keys");
    if (kuExtensions.testFlag(Okular::CertificateInfo::KuDataEncipherment))
        ku << i18n("Decrypt Keys");
    if (kuExtensions.testFlag(Okular::CertificateInfo::KuKeyAgreement))
        ku << i18n("Key Agreement");
    if (kuExtensions.testFlag(Okular::CertificateInfo::KuKeyCertSign))
        ku << i18n("Sign Certificate");
    if (kuExtensions.testFlag(Okular::CertificateInfo::KuClrSign))
        ku << i18n("Sign CRL");
    if (kuExtensions.testFlag(Okular::CertificateInfo::KuEncipherOnly))
        ku << i18n("Encrypt Only");
    if (ku.isEmpty())
        ku << i18n("No Usage Specified");
    return ku.join(separator);
}

QString getReadableKeyUsageCommaSeparated(Okular::CertificateInfo::KeyUsageExtensions kuExtensions)
{
    return getReadableKeyUsage(kuExtensions, i18nc("Joins the various ways a signature key can be used in a longer string", ", "));
}

QString getReadableKeyUsageNewLineSeparated(Okular::CertificateInfo::KeyUsageExtensions kuExtensions)
{
    return getReadableKeyUsage(kuExtensions, QStringLiteral("\n"));
}

std::unique_ptr<Okular::CertificateInfo> getCertificateAndPasswordForSigning(PageView *pageView, Okular::Document *doc, QString *password, QString *documentPassword)
{
    const Okular::CertificateStore *certStore = doc->certificateStore();
    bool userCancelled, nonDateValidCerts;
    QList<Okular::CertificateInfo *> certs = certStore->signingCertificatesForNow(&userCancelled, &nonDateValidCerts);
    if (userCancelled) {
        return nullptr;
    }

    if (certs.isEmpty()) {
        pageView->showNoSigningCertificatesDialog(nonDateValidCerts);
        return nullptr;
    }

    QStringList items;
    QHash<QString, Okular::CertificateInfo *> nickToCert;
    for (auto cert : qAsConst(certs)) {
        items.append(cert->nickName());
        nickToCert[cert->nickName()] = cert;
    }

    bool resok = false;
    const QString certNicknameToUse = QInputDialog::getItem(pageView, i18n("Select certificate to sign with"), i18n("Certificates:"), items, 0, false, &resok);

    if (!resok) {
        qDeleteAll(certs);
        return nullptr;
    }

    // I could not find any case in which i need to enter a password to use the certificate, seems that once you unlcok the firefox/NSS database
    // you don't need a password anymore, but still there's code to do that in NSS so we have code to ask for it if needed. What we do is
    // ask if the empty password is fine, if it is we don't ask the user anything, if it's not, we ask for a password
    Okular::CertificateInfo *cert = nickToCert.value(certNicknameToUse);
    bool passok = cert->checkPassword(*password);
    while (!passok) {
        const QString title = i18n("Enter password (if any) to unlock certificate: %1", certNicknameToUse);
        bool ok;
        *password = QInputDialog::getText(pageView, i18n("Enter certificate password"), title, QLineEdit::Password, QString(), &ok);
        if (ok) {
            passok = cert->checkPassword(*password);
        } else {
            passok = false;
            break;
        }
    }

    if (doc->metaData(QStringLiteral("DocumentHasPassword")).toString() == QLatin1String("yes")) {
        *documentPassword = QInputDialog::getText(pageView, i18n("Enter document password"), i18n("Enter document password"), QLineEdit::Password, QString(), &passok);
    }

    if (passok) {
        certs.removeOne(cert);
    }
    qDeleteAll(certs);

    return passok ? std::unique_ptr<Okular::CertificateInfo>(cert) : std::unique_ptr<Okular::CertificateInfo>();
}

QString getFileNameForNewSignedFile(PageView *pageView, Okular::Document *doc)
{
    QMimeDatabase db;
    const QString typeName = doc->documentInfo().get(Okular::DocumentInfo::MimeType);
    const QMimeType mimeType = db.mimeTypeForName(typeName);
    const QString mimeTypeFilter = i18nc("File type name and pattern", "%1 (%2)", mimeType.comment(), mimeType.globPatterns().join(QLatin1Char(' ')));

    const QUrl currentFileUrl = doc->currentDocument();
    const QFileInfo currentFileInfo(currentFileUrl.fileName());
    const QString localFilePathIfAny = currentFileUrl.isLocalFile() ? QFileInfo(currentFileUrl.path()).canonicalPath() + QLatin1Char('/') : QString();
    const QString newFileName =
        localFilePathIfAny + i18nc("Used when suggesting a new name for a digitally signed file. %1 is the old file name and %2 it's extension", "%1_signed.%2", currentFileInfo.baseName(), currentFileInfo.completeSuffix());

    return QFileDialog::getSaveFileName(pageView, i18n("Save Signed File As"), newFileName, mimeTypeFilter);
}

void signUnsignedSignature(const Okular::FormFieldSignature *form, PageView *pageView, Okular::Document *doc)
{
    Q_ASSERT(form && form->signatureType() == Okular::FormFieldSignature::UnsignedSignature);
    QString password, documentPassword;
    const std::unique_ptr<Okular::CertificateInfo> cert = SignatureGuiUtils::getCertificateAndPasswordForSigning(pageView, doc, &password, &documentPassword);
    if (!cert) {
        return;
    }

    Okular::NewSignatureData data;
    data.setCertNickname(cert->nickName());
    data.setCertSubjectCommonName(cert->subjectInfo(Okular::CertificateInfo::CommonName));
    data.setPassword(password);
    data.setDocumentPassword(documentPassword);
    password.clear();
    documentPassword.clear();

    const QString newFilePath = SignatureGuiUtils::getFileNameForNewSignedFile(pageView, doc);

    if (!newFilePath.isEmpty()) {
        const bool success = form->sign(data, newFilePath);
        if (success) {
            emit pageView->requestOpenFile(newFilePath, form->page()->number() + 1);
        } else {
            KMessageBox::error(pageView, i18nc("%1 is a file path", "Could not sign. Invalid certificate password or could not write to '%1'", newFilePath));
        }
    }
}
}
